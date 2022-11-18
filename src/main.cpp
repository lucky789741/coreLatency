#include <iostream>
#include <iomanip>
#include <fstream>
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
    std::string fileName;
    std::fstream fs;
    if (vm.count("output"))
    {
        fileName = vm["output"].as<std::string>();
        fs.open(fileName, std::ios::out | std::ios::trunc);
    }
    DWORD64 iters = 100000;
    if (vm.count("iters")) iters = vm["iters"].as<DWORD64>();

    Utils::setHighestPriority();
    DWORD cpuCount = Utils::getCPUCount();
    std::barrier syncPoint(2);
    std::chrono::high_resolution_clock clock = std::chrono::high_resolution_clock();
    
    std::cout << "CPU: " << Utils::getCPUName() << std::endl;
    std::cout << "Number of Logical Processors: " << cpuCount << std::endl;
    std::cout << "Number of iterations: " << iters << '\n' << std::endl;
    std::cout << std::setw(4) <<' ';
    for (DWORD i = 0; i < cpuCount; i++)
        std::cout << std::setw(4) << i;
    std::cout << '\n';
    DWORD64 lat;
    for (DWORD i = 0; i < cpuCount; i++)
    {
        std::cout <<std::setw(4) << i;
        for (DWORD j = 0; j <= i; j++)
        {
            if (i == j)
            {
                std::cout << std::setw(4) << "0";
                fs << '0' << ',';
                continue;
            }
            lat = Bench::pingPong(i, j, iters, syncPoint, clock);
            std::cout << std::setw(4) << lat;
            fs << lat << ',';
        }
        std::cout << '\n';
        fs << '\n';
    }
    if (fs.is_open()) fs.close();

}

