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
        DWORD64 iters = 100;
        DWORD64 warmupIters = 0; // 0 means same as iters
        DWORD64 samples = 100;
        size_t maxSizeKB = 262144;
        std::string outputFile;
        bool listSizes = false;
        int targetCore = -1; // -1 means all cores

        po::options_description desc("Available options");
        desc.add_options()
            ("help", "Print this message")
            ("iters", po::value<DWORD64>(), "Pointer loads per measurement (default: 100)")
            ("samples", po::value<DWORD64>(), "Measurements per (core, size), median reported (default: 100)")
            ("warmup", po::value<DWORD64>(), "Warmup loads before timing (default: same as --iters)")
            ("max-size", po::value<size_t>(), "Max buffer size in KB, must be a valid log-scale step (default: 262144)")
            ("list-sizes", po::bool_switch(&listSizes), "Print valid buffer sizes in KB and exit")
            ("core", po::value<int>(), "Test only the specified core (default: all cores)")
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
        if (vm.count("core")) {
            targetCore = vm["core"].as<int>();
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

        // Validate --core
        if (targetCore >= (int)cpuCount) {
            std::cerr << "Error: core " << targetCore
                      << " out of range (0-" << cpuCount - 1 << ")\n";
            return 1;
        }
        DWORD firstCore = (targetCore >= 0) ? (DWORD)targetCore : 0;
        DWORD lastCore  = (targetCore >= 0) ? (DWORD)targetCore : cpuCount - 1;

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

        // CSV header for streaming output
        std::cout << "core,size(KB),ns\n";

        // Per-core outer loop: one core finishes all sizes before next core
        DWORD coreCount = lastCore - firstCore + 1;

        // Open output file early for streaming write
        std::ofstream fs;
        if (!outputFile.empty()) {
            fs.open(outputFile, std::ios::out | std::ios::trunc);
            if (!fs.is_open())
                throw std::runtime_error("Unable to write to: " + outputFile);
            fs << "core,size(KB),ns\n";
        }

        for (DWORD ci = 0; ci < coreCount; ci++) {
            DWORD core = firstCore + ci;
            Utils::setAffinity(core);

            for (size_t si = 0; si < validSizes.size(); si++) {
                std::vector<DOUBLE> sampleDurations;
                sampleDurations.reserve(samples);
                for (DWORD64 sm = 0; sm < samples; sm++) {
                    sampleDurations.push_back(
                        MemBench::measure(validSizes[si], cacheLine,
                                          iters, warmupIters));
                }
                DOUBLE lat = Utils::median(sampleDurations);
                DOUBLE sizeKB = (DOUBLE)validSizes[si] / 1024.0;

                std::cout << core << ','
                          << std::fixed << std::setprecision(1) << sizeKB << ','
                          << std::fixed << std::setprecision(1) << lat << '\n';

                if (fs.is_open()) {
                    fs << core << ','
                       << std::fixed << std::setprecision(1) << sizeKB << ','
                       << std::fixed << std::setprecision(1) << lat << '\n';
                }
            }
        }

        if (fs.is_open()) {
            fs.close();
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
