#pragma once
#include <Windows.h>
#include <vector>

#define STRIDE_DEFAULT  (512 / sizeof(char*))

namespace MemBench
{
    DWORD getCacheLineSize();
    size_t step(size_t k);
    std::vector<size_t> generateValidSizes(size_t maxSizeKB);
    DOUBLE measure(size_t bufferSize, DWORD stride, DWORD64 iters,
                   DWORD64 warmupIters);
}
