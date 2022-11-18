#pragma once
#include <Windows.h>
#include <string>

namespace Utils
{
	std::string getCPUName();
	DWORD getCPUCount();
	VOID setAffinity(DWORD coreID);
	VOID setHighestPriority();
}