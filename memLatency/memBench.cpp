#include "memBench.h"
#include "..\src\utils.h"
#include <chrono>
#include <random>
#include <stdexcept>

namespace MemBench
{

DWORD getCacheLineSize()
{
    DWORD size = 0;
    GetLogicalProcessorInformation(nullptr, &size);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(
        size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(buf.data(), &size))
        return STRIDE_DEFAULT;

    for (const auto& info : buf)
    {
        if (info.Relationship == RelationCache &&
            info.Cache.Level == 1)
        {
            return info.Cache.LineSize;
        }
    }
    return STRIDE_DEFAULT;
}

// Ported from lmbench lat_mem_rd.c
size_t step(size_t k)
{
    if (k < 1024) {
        k = k * 2;
    } else if (k < 4 * 1024) {
        k += 1024;
    } else {
        size_t s;
        for (s = 32 * 1024; s <= k; s *= 2)
            ;
        k += s / 16;
    }
    return k;
}

std::vector<size_t> generateValidSizes(size_t maxSizeKB)
{
    std::vector<size_t> sizes;
    size_t maxBytes = maxSizeKB * 1024ULL;
    size_t k = 512;
    while (k <= maxBytes) {
        sizes.push_back(k);
        k = step(k);
    }
    return sizes;
}

static void buildChain(char* buf, size_t bufSize, DWORD stride)
{
    size_t N = bufSize / stride;
    if (N < 2) return;

    std::vector<size_t> indices(N);
    for (size_t i = 0; i < N; i++)
        indices[i] = i;

    // Fisher-Yates shuffle
    static thread_local std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    for (size_t i = N - 1; i > 0; i--) {
        size_t j = rng() % (i + 1);
        std::swap(indices[i], indices[j]);
    }

    for (size_t i = 0; i < N; i++) {
        size_t next = indices[(i + 1) % N];
        *(char**)(buf + i * stride) = buf + next * stride;
    }
}

#define ONE   p = (char**)*p;
#define FIVE  ONE ONE ONE ONE ONE
#define TEN   FIVE FIVE

// Volatile sink prevents LTCG from optimizing away the pointer chase.
// __declspec(noinline) is not enough when /LTCG is enabled.
static char** volatile g_sink;

DOUBLE measure(size_t bufferSize, DWORD stride, DWORD64 iters,
               DWORD64 warmupIters)
{
    size_t slotCount = bufferSize / stride;
    if (slotCount < 2)
        slotCount = 2;
    size_t adjustedSize = slotCount * stride;

    char* buf = (char*)VirtualAlloc(nullptr, adjustedSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buf)
        throw std::bad_alloc();

    buildChain(buf, adjustedSize, stride);
    char** p = (char**)buf;

    // Warmup: full-chain traversals, unrolled
    DWORD64 warmupLoads = warmupIters;
    while (warmupLoads >= 10) {
        TEN;
        warmupLoads -= 10;
    }
    while (warmupLoads-- > 0) ONE;

    // Timed traversal
    DWORD64 remaining = iters;
    auto start = std::chrono::high_resolution_clock::now();
    while (remaining >= 10) {
        TEN;
        remaining -= 10;
    }
    while (remaining-- > 0) ONE;
    auto end = std::chrono::high_resolution_clock::now();

    // Prevent elimination: store p to volatile global
    g_sink = p;

    VirtualFree(buf, 0, MEM_RELEASE);

    DOUBLE duration = (DOUBLE)(end - start).count() / iters;
    return duration;
}

} // namespace MemBench
