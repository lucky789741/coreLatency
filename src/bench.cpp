#include "bench.h"
#include "utils.h"

namespace Bench
{

    BOOL casSet(std::atomic<State>& state, State expected, State desired)
    {
        return state.compare_exchange_weak(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    DOUBLE pingPong(DWORD pingCore, DWORD pongCore, DWORD64 iters,
        std::barrier<std::_No_completion_function>& syncPoint,
        std::chrono::high_resolution_clock& clock)
    {
        std::chrono::high_resolution_clock::time_point start, end;
        DOUBLE duration;
        DWORD64 iterations = iters;
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
        start = clock.now();
        while (iterations--)
        {
            //same thing
            //keep trying to change state to PING if state is PONG
            //this thread change it to PING and far thread change it to PONG
            //so we'll measure two way latency
            while (!casSet(state, PONG, PING)) {}
        }
        end = clock.now();
        
        //try to flip it to FINISH to end far thread's loop
        while (!casSet(state, PONG, FINISH)) {}

        pong.join();

        //div it by 2.0 to get one way latency
        duration = (DOUBLE)(end - start).count() / 2.0 / iters;

        return duration;

    };
}