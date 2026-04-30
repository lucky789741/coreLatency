#pragma once
#include <Windows.h>
#include <atomic>
#include <barrier>
#include <thread>

enum State :CHAR
{
    FINISH,
    PING,
    PONG,
};

namespace Bench
{
    BOOL casSet(std::atomic<State>& state, State expected, State desired);
    DOUBLE pingPong(DWORD pingCore, DWORD pongCore, DWORD64 iters, DWORD64 samples, DWORD64 warmupIters,
        std::barrier<std::_No_completion_function>& syncPoint);
}