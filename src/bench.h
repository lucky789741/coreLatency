#pragma once
#include <Windows.h>
#include <atomic>
#include <barrier>
#include <chrono>

enum State :CHAR
{
    FINISH,
    PING,
    PONG,
};

namespace Bench
{
    //wrapper for compare_exchange_weak
    //return true if state's current value is eqaul to expected and it will change value to newState(desired)
	BOOL casSet(std::atomic<State>& state, State expected, State desired);
    DOUBLE pingPong(DWORD pingCore, DWORD pongCore, DWORD iters,
        std::barrier<std::_No_completion_function>& syncPoint,
        std::chrono::high_resolution_clock& clock);
}