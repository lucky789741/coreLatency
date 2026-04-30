#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include "utils.h"
#include "bench.h"

namespace po = boost::program_options;

int main(INT argc, CHAR* argv[])
{
    try
    {
        po::options_description desc("Available options");
        desc.add_options()
            ("help", "Print this message")
            ("output", po::value<std::string>(), "Output result to specified csv file name.\n!!WILL OVERWRITE THE FILE WITH SAME NAME!!")
            ("iters", po::value<DWORD64>(), "Iteration per samples.\n(default:1000)")
            ("samples", po::value<DWORD64>(), "More sample lead to better result\n(default:300)")
            ("warmup", po::value<DWORD64>(), "Warmup iterations before measurement.\n(default: same as --iters)")
            ;
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 1;
        }
        DWORD64 iters = 1000;
        if (vm.count("iters"))
            iters = vm["iters"].as<DWORD64>();
        DWORD64 samples = 300;
        if (vm.count("samples"))
            samples = vm["samples"].as<DWORD64>();
        DWORD64 warmupIters = iters;
        if (vm.count("warmup"))
            warmupIters = vm["warmup"].as<DWORD64>();

        Utils::initCoreTopology();
        Utils::setHighestPriority();
        DWORD cpuCount = Utils::getCPUCount();
        std::barrier syncPoint(2);
        std::vector<std::vector<DOUBLE>> result;

        std::cout << "CPU: " << Utils::getCPUName() << std::endl;
        std::cout << "Num of CPUs: " << cpuCount << std::endl;
        std::cout << "Num of iters: " << iters << std::endl;
        std::cout << "Num of samples: " << samples << std::endl;
        std::cout << "Warmup iters: " << warmupIters << '\n' << std::endl;

        // Show core topology
        const auto& topo = Utils::getCoreTopology();
        std::cout << "Core topology (efficiency class): ";
        for (DWORD i = 0; i < cpuCount; i++)
            std::cout << i << ':' << (INT)topo[i].efficiencyClass << ' ';
        std::cout << "\n" << std::endl;

        std::cout << std::setw(5) << ' ';
        for (DWORD i = 0; i < cpuCount; i++)
            std::cout << std::setw(6) << i;
        std::cout << '\n';
        DOUBLE lat;
        for (DWORD i = 0; i < cpuCount; i++)
        {
            std::cout << std::setw(5) << i;
            result.push_back(std::vector<DOUBLE>());
            for (DWORD j = 0; j <= i; j++)
            {
                if (i == j)
                {
                    std::cout << std::setw(6) << " ";
                    result[i].push_back(0);
                    continue;
                }
                lat = Bench::pingPong(i, j, iters, samples, warmupIters, syncPoint);
                result[i].push_back(lat);
                std::cout << std::setw(6) << std::fixed << std::setprecision(1) << lat;
            }
            std::cout << '\n';
        }

        if (vm.count("output"))
            Utils::saveCSV(vm["output"].as<std::string>(), result);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}