#include "bench.h"
#include "utils.h"
#include <chrono>

namespace Bench
{

    BOOL casSet(std::atomic<State>& state, State expected, State desired)
    {
        return state.compare_exchange_weak(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    DOUBLE pingPong(DWORD pingCore, DWORD pongCore, DWORD64 iters, DWORD64 samples, DWORD64 warmupIters,
        std::barrier<std::_No_completion_function>& syncPoint)
    {
        std::vector<DOUBLE> durationSamples;
        Utils::setAffinity(pingCore);
        alignas(64) std::atomic<State> state = PONG;

        std::thread pong([&]()
            {
                Utils::setAffinity(pongCore);
                syncPoint.arrive_and_wait();

                while (true)
                {
                    while (!casSet(state, PING, PONG))
                    {
                        if (state.load(std::memory_order_relaxed) == FINISH) return;
                    }
                }
            });
        syncPoint.arrive_and_wait();

        // Warmup phase — establish cache coherency before timing
        for (DWORD64 w = 0; w < warmupIters; w++)
        {
            while (!casSet(state, PONG, PING)) {}
        }

        for (DWORD64 j = 0; j < samples; j++)
        {
            DWORD64 i = iters;
            auto start = std::chrono::high_resolution_clock::now();
            while (i--)
            {
                while (!casSet(state, PONG, PING)) {}
            }
            auto end = std::chrono::high_resolution_clock::now();
            DOUBLE duration = (DOUBLE)(end - start).count() / 2.0 / iters;
            durationSamples.push_back(duration);
        }

        while (!casSet(state, PONG, FINISH)) {}

        pong.join();

        return Utils::median(durationSamples);
    };
}