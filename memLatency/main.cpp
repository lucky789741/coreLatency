#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <boost/program_options.hpp>
#include "..\src\utils.h"
#include "memBench.h"

namespace po = boost::program_options;

static void printValidSizes(size_t maxSizeKB = 262144)
{
    auto sizes = MemBench::generateValidSizes(maxSizeKB);
    std::cout << "Valid buffer sizes (KB):\n";
    for (auto b : sizes) {
        DOUBLE kb = (DOUBLE)b / 1024.0;
        std::cout << std::fixed << std::setprecision(1) << kb << '\n';
    }
}

static bool isValidMaxSize(size_t sizeKB)
{
    auto sizes = MemBench::generateValidSizes(sizeKB);
    return !sizes.empty() && sizes.back() == sizeKB * 1024ULL;
}

static void printNearestSizes(size_t sizeKB)
{
    // Generate a broad set up to 1 TB and find nearest valid sizes
    auto sizes = MemBench::generateValidSizes(1048576);
    std::vector<DOUBLE> sizesKB;
    for (auto b : sizes)
        sizesKB.push_back((DOUBLE)b / 1024.0);

    DOUBLE target = (DOUBLE)sizeKB;
    size_t closestIdx = 0;
    DOUBLE closestDiff = std::abs(sizesKB[0] - target);
    for (size_t i = 1; i < sizesKB.size(); i++) {
        DOUBLE diff = std::abs(sizesKB[i] - target);
        if (diff < closestDiff) {
            closestDiff = diff;
            closestIdx = i;
        }
    }

    std::cerr << "Nearest valid sizes (KB):\n";
    if (closestIdx > 0)
        std::cerr << "  " << sizesKB[closestIdx - 1] << '\n';
    std::cerr << "  " << sizesKB[closestIdx] << '\n';
    if (closestIdx + 1 < sizesKB.size())
        std::cerr << "  " << sizesKB[closestIdx + 1] << '\n';
}

int main(INT argc, CHAR* argv[])
{
    try
    {
        DWORD64 iters = 1000;
        DWORD64 samples = 300;
        DWORD64 warmupIters = 0; // 0 means same as iters
        size_t maxSizeKB = 262144;
        std::string outputFile;
        bool listSizes = false;

        po::options_description desc("Available options");
        desc.add_options()
            ("help", "Print this message")
            ("iters", po::value<DWORD64>(), "Pointer loads per sample (default: 1000)")
            ("samples", po::value<DWORD64>(), "Timed batches per core per buffer size (default: 300)")
            ("warmup", po::value<DWORD64>(), "Warmup loads before timing (default: same as --iters)")
            ("max-size", po::value<size_t>(), "Max buffer size in KB, must be a valid log-scale step (default: 262144)")
            ("list-sizes", po::bool_switch(&listSizes), "Print valid buffer sizes in KB and exit")
            ("output", po::value<std::string>(), "Save CSV to file (overwrites if exists)")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            std::cout << "All buffer sizes are in KB (kilobytes).\n";
            return 0;
        }

        if (vm.count("iters"))
            iters = vm["iters"].as<DWORD64>();
        if (vm.count("samples"))
            samples = vm["samples"].as<DWORD64>();
        if (vm.count("warmup"))
            warmupIters = vm["warmup"].as<DWORD64>();
        else
            warmupIters = iters;
        if (vm.count("max-size")) {
            maxSizeKB = vm["max-size"].as<size_t>();
        }
        if (vm.count("output"))
            outputFile = vm["output"].as<std::string>();

        // --list-sizes: print all valid buffer sizes and exit (up to 1 TB)
        if (listSizes) {
            printValidSizes(1048576);
            return 0;
        }

        // Validate --max-size
        if (!isValidMaxSize(maxSizeKB)) {
            std::cerr << "Error: " << maxSizeKB
                      << " KB is not a valid log-scale buffer size.\n";
            printNearestSizes(maxSizeKB);
            return 1;
        }

        // Init topology and detect cache line
        Utils::initCoreTopology();
        Utils::setHighestPriority();
        DWORD cpuCount = Utils::getCPUCount();
        DWORD cacheLine = MemBench::getCacheLineSize();

        // Generate valid buffer sizes in bytes
        auto validSizes = MemBench::generateValidSizes(maxSizeKB);

        // Print header info
        std::cout << "CPU: " << Utils::getCPUName() << std::endl;
        std::cout << "Cores: " << cpuCount << std::endl;
        std::cout << "Cache line: " << cacheLine << " bytes" << std::endl;
        std::cout << "Iters: " << iters << ", Samples: " << samples
                  << ", Warmup: " << warmupIters << std::endl;
        std::cout << "Max buffer size (KB): " << maxSizeKB << '\n' << std::endl;

        const auto& topo = Utils::getCoreTopology();
        std::cout << "Core topology (efficiency class): ";
        for (DWORD i = 0; i < cpuCount; i++)
            std::cout << i << '(' << (INT)topo[i].efficiencyClass << ") ";
        std::cout << "\n" << std::endl;

        // CSV header
        std::cout << "size(KB)";
        for (DWORD c = 0; c < cpuCount; c++)
            std::cout << ',' << 'c' << c << '(' << (INT)topo[c].efficiencyClass << ')';
        std::cout << '\n';

        // Per-core per-size measurement
        std::vector<std::vector<DOUBLE>> results;
        for (const auto& sizeBytes : validSizes) {
            std::vector<DOUBLE> row;
            row.reserve(cpuCount);

            for (DWORD core = 0; core < cpuCount; core++) {
                Utils::setAffinity(core);
                DOUBLE lat = MemBench::measure(sizeBytes, cacheLine,
                                               iters, warmupIters, samples);
                row.push_back(lat);
            }

            results.push_back(row);
        }

        // Print CSV body
        auto csvOut = [&](std::ostream& os) {
            for (size_t r = 0; r < validSizes.size(); r++) {
                DOUBLE sizeKB = (DOUBLE)validSizes[r] / 1024.0;
                os << std::fixed << std::setprecision(1) << sizeKB;
                for (DWORD c = 0; c < cpuCount; c++) {
                    os << ',' << std::fixed << std::setprecision(1)
                       << results[r][c];
                }
                os << '\n';
            }
        };

        csvOut(std::cout);

        if (!outputFile.empty()) {
            std::ofstream fs(outputFile, std::ios::out | std::ios::trunc);
            if (!fs.is_open())
                throw std::runtime_error("Unable to write to: " + outputFile);
            csvOut(fs);
            std::cerr << "Results saved to " << outputFile << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
