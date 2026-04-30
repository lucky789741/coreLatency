#include "utils.h"
#include <intrin.h>
#include <fstream>
#include <system_error>

namespace Utils
{
    static std::vector<CoreInfo> s_coreInfos;

    VOID initCoreTopology()
    {
        s_coreInfos.clear();

        ULONG size = 0;
        if (!GetSystemCpuSetInformation(nullptr, 0, &size, GetCurrentProcess(), 0) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            // Fallback: pre-Win10, use GetActiveProcessorCount
            DWORD count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            for (DWORD i = 0; i < count; i++)
            {
                CoreInfo ci;
                ci.flatId = i;
                ci.group = 0;
                ci.number = (BYTE)i;
                ci.efficiencyClass = 0;
                s_coreInfos.push_back(ci);
            }
            return;
        }

        std::vector<BYTE> buf(size);
        auto* sets = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buf.data());
        if (!GetSystemCpuSetInformation(sets, size, &size, GetCurrentProcess(), 0))
            return;

        DWORD flatId = 0;
        ULONG count = size / sizeof(SYSTEM_CPU_SET_INFORMATION);
        for (ULONG i = 0; i < count; i++)
        {
            if (sets[i].Type != CpuSetInformation)
                continue;
            CoreInfo ci;
            ci.flatId = flatId++;
            ci.group = sets[i].CpuSet.Group;
            ci.number = (BYTE)sets[i].CpuSet.LogicalProcessorIndex;
            ci.efficiencyClass = sets[i].CpuSet.EfficiencyClass;
            s_coreInfos.push_back(ci);
        }
    }

    std::string getCPUName()
    {
        INT cpuInfo[4];
        std::string brandString;
        brandString.reserve(0x40);

        for (INT functionID = 0x80000002; functionID < 0x80000005; functionID++)
        {
            __cpuid(cpuInfo, functionID);
            std::string temp = (CHAR*)cpuInfo;
            brandString += temp.substr(0, 16);
        }
        return brandString;
    }

    DWORD getCPUCount()
    {
        return (DWORD)s_coreInfos.size();
    }

    const std::vector<CoreInfo>& getCoreTopology()
    {
        return s_coreInfos;
    }

    VOID setAffinity(DWORD coreID)
    {
        if (coreID >= s_coreInfos.size())
            throw std::out_of_range("coreID out of range");

        const auto& ci = s_coreInfos[coreID];

        GROUP_AFFINITY ga = {};
        ga.Group = ci.group;
        ga.Mask = 1ULL << ci.number;

        GROUP_AFFINITY prev;
        if (!SetThreadGroupAffinity(GetCurrentThread(), &ga, &prev))
            throw std::system_error(GetLastError(), std::system_category(), "SetThreadGroupAffinity failed");

        // Verify the thread actually migrated to the target core
        PROCESSOR_NUMBER pn;
        for (INT retry = 0; retry < 1000; retry++)
        {
            GetCurrentProcessorNumberEx(&pn);
            if (pn.Group == ci.group && pn.Number == ci.number)
                return;
            // Combine Sleep(0) and Sleep(1) — yield quantum but don't wait too long
            if (retry < 10)
                Sleep(0);
            else
                Sleep(1);
        }
    }

    VOID setHighestPriority()
    {
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
            throw std::system_error(GetLastError(), std::system_category(), "SetPriorityClass failed");
    }

    VOID saveCSV(const std::string& fileName, const std::vector<std::vector<DOUBLE>>& result)
    {
        std::fstream fs;
        fs.open(fileName, std::ios::out | std::ios::trunc);
        if (!fs.is_open())
            throw std::runtime_error("unable to write to file: " + fileName);

        size_t maxLen = result.size();

        for (size_t i = 0; i < maxLen; i++)
        {
            for (size_t j = 0; j < maxLen; j++)
            {
                if (j < i)
                    fs << result[i][j];
                if (j != maxLen - 1)
                    fs << ',';
            }
            fs << '\n';
        }

        if (fs.is_open()) fs.close();
    }
}