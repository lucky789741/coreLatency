#include "utils.h"
#include <intrin.h>
#include <fstream>
namespace Utils
{
    std::string getCPUName()
    {
        INT cpuInfo[4];
        std::string brandString;
        brandString.reserve(0x40);

        for (INT functionID = 0x80000002; functionID < 0x80000005; functionID++)
        {
            __cpuid(cpuInfo, functionID);
            std::string temp = (CHAR*)cpuInfo;
            brandString += temp.substr(0,16);
        }
        return brandString;
    }
    DWORD getCPUCount()
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        return sysInfo.dwNumberOfProcessors;
    }

    VOID setAffinity(DWORD coreID)
    {
        DWORD_PTR mask = (DWORD_PTR)1 << coreID;
        if (!SetThreadAffinityMask(GetCurrentThread(), mask))
        {
            throw GetLastError();
        }
        Sleep(100);
    }
    VOID setHighestPriority()
    {
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
        {
            throw GetLastError();
        }
    }

    VOID saveCSV(const std::string& fileName,std::vector<std::vector<WORD>>& result)
    {
        std::fstream fs;
        fs.open(fileName, std::ios::out | std::ios::trunc);
        if (!fs.is_open()) throw "unable write to file";

        DWORD maxLen = result.size();

        for (DWORD i = 0; i < maxLen; i++)
        {
            for (DWORD j = 0; j < maxLen; j++)
            {
                if(j<i)
                    fs << result[i][j];
                if (j != maxLen - 1)
                    fs << ',';
            }
            fs << '\n';
        }

        if (fs.is_open()) fs.close();
    }
}