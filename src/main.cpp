#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include "utils.h"
#include "bench.h"

namespace po = boost::program_options;

int main(INT argc,CHAR* argv[])
{
    //parse options
    po::options_description desc("Available options");
    desc.add_options()
        ("help","Print this message")
        ("output",po::value<std::string>(),"Output result to specified csv file name.\n!!WILL OVERWRITE THE FILE WITH SAME NAME!!")
        ("iters",po::value<DWORD64>(),"More iteration may lead to accurate result.\n(default:100000)")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }
    DWORD64 iters = 100000;
    if (vm.count("iters"))
        iters = vm["iters"].as<DWORD64>();

    Utils::setHighestPriority();
    DWORD cpuCount = Utils::getCPUCount();
    std::barrier syncPoint(2);
    std::chrono::high_resolution_clock clock = std::chrono::high_resolution_clock();
    std::vector<std::vector<WORD>> result;
    
    std::cout << "CPU: " << Utils::getCPUName() << std::endl;
    std::cout << "Number of Logical Processors: " << cpuCount << std::endl;
    std::cout << "Number of iterations: " << iters << '\n' << std::endl;
    std::cout << std::setw(4) <<' ';
    for (DWORD i = 0; i < cpuCount; i++)
        std::cout << std::setw(4) << i;
    std::cout << '\n';
    WORD lat;
    for (DWORD i = 0; i < cpuCount; i++)
    {
        std::cout <<std::setw(4) << i;
        result.push_back(std::vector<WORD>());
        for (DWORD j = 0; j <= i; j++)
        {
            if (i == j)
            {
                std::cout << std::setw(4) << " ";
                result[i].push_back(0);
                continue;
            }
            lat = Bench::pingPong(i, j, iters, syncPoint, clock);
            result[i].push_back(lat);
            std::cout << std::setw(4) << lat;
        }
        std::cout << '\n';
    }
    
    if (vm.count("output"))
        Utils::saveCSV(vm["output"].as<std::string>(),result);
}

