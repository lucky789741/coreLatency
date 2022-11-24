#include "bench.h"
#include "utils.h"
#include <numeric>

namespace Bench
{

    BOOL casSet(std::atomic<State>& state, State expected, State desired)
    {
        return state.compare_exchange_weak(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    DOUBLE pingPong(DWORD pingCore, DWORD pongCore, DWORD64 iters, DWORD64 samples,
        std::barrier<std::_No_completion_function>& syncPoint,
        std::chrono::high_resolution_clock& clock)
    {
        //extra warm up
        samples++;
        std::chrono::high_resolution_clock::time_point start, end;
        std::vector<DOUBLE> durationSamples;
        DOUBLE duration;
        Utils::setAffinity(pingCore);
        alignas(64) std::atomic<State> state = PONG;

        std::thread pong([&]()
            {
                Utils::setAffinity(pongCore);
                //barrier make sure both thread changed Affinity before start
                syncPoint.arrive_and_wait();

                //it will stuck at inner loop if check state at outer loop
                while (true)
                {
                    //because it return true if state == PING and try to change it to PONG
                    //after change to PONG it will jump to outer loop
                    while (!casSet(state, PING, PONG))
                    {
                        if (state.load(std::memory_order_relaxed) == FINISH) return;
                    }
                }
            });
        //barrier make sure both thread changed Affinity before start
        syncPoint.arrive_and_wait();
        for (DWORD64 j = 0; j < samples; j++) {
            DWORD64 i = iters;
            start = clock.now();
            while (i--)
            {
                //same thing
                //keep trying to change state to PING if state is PONG
                //this thread change it to PING and far thread change it to PONG
                //so we'll measure two way latency
                while (!casSet(state, PONG, PING)) {}
            }
            end = clock.now();
            //div it by 2.0 to get one way latency
            duration = (DOUBLE)(end - start).count() / 2.0 / iters;
            durationSamples.push_back(duration);
        }
        
        //try to flip it to FINISH to end far thread's loop
        while (!casSet(state, PONG, FINISH)) {}

        pong.join();

        duration = std::accumulate(durationSamples.begin()+1,durationSamples.end(),0.0);

        duration = duration / (samples - 1);

        return duration;

    };
}