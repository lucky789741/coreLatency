#include "utils.h"
#include <intrin.h>

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
    }
    VOID setHighestPriority()
    {
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
        {
            throw GetLastError();
        }
    }
}