#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

struct CoreInfo
{
    DWORD flatId;
    WORD group;
    BYTE number;
    BYTE efficiencyClass;
};

namespace Utils
{
    VOID initCoreTopology();
    std::string getCPUName();
    DWORD getCPUCount();
    const std::vector<CoreInfo>& getCoreTopology();
    VOID setAffinity(DWORD coreID);
    VOID setHighestPriority();
    VOID saveCSV(const std::string& filename, const std::vector<std::vector<DOUBLE>>& result);

    template<typename T>
    T median(std::vector<T> v)
    {
        if (v.empty()) return T{};
        size_t n = v.size() / 2;
        std::nth_element(v.begin(), v.begin() + n, v.end());
        return v[n];
    }
}