#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace Utils
{
	std::string getCPUName();
	DWORD getCPUCount();
	VOID setAffinity(DWORD coreID);
	VOID setHighestPriority();
	VOID saveCSV(const std::string& filename, std::vector<std::vector<WORD>>& result);
}