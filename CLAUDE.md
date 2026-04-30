# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
# From a VS 2026 x64 Native Tools command prompt:
msbuild coreLatency.sln /p:Configuration=Release /p:Platform=x64
```

NuGet package restore is required before first build. The project includes nuget.exe in the root:

```bash
.\nuget.exe restore coreLatency.sln
```

## Architecture

A Windows console application that measures inter-core CPU latency via atomic CAS ping-pong. C++20, MSVC v145 toolset (VS 2026), Boost 1.80.0 for CLI arg parsing.

**Measurement technique**: Two threads are pinned to specific cores via `SetThreadGroupAffinity` (supports >64 cores via processor groups). They bounce an `alignas(64)` atomic `State` enum (PING/PONG) back and forth using `compare_exchange_weak(memory_order_relaxed)`. Each round-trip batch is timed with `std::chrono::high_resolution_clock`. Result is one-way latency in nanoseconds: `total_time / 2 / iters`. The median across all samples is reported (robust against OS interrupt outliers). Results form a lower-triangular matrix printed to stdout and optionally saved as CSV.

**Files**:
- `src/main.cpp` — CLI entry point. Parses `--iters`, `--samples`, `--warmup`, `--output`, `--help` via Boost program_options. Calls `Utils::initCoreTopology()` then iterates over all core pairs.
- `src/bench.h` / `src/bench.cpp` — `Bench::pingPong()`: spawns the pong thread, runs warmup phase, then timed CAS rounds. Returns `Utils::median()` of all sample durations.
- `src/utils.h` / `src/utils.cpp` — Core topology via `GetSystemCpuSetInformation`, affinity via `SetThreadGroupAffinity` + `GetCurrentProcessorNumberEx` verification loop, CPU name via `__cpuid`, priority elevation, CSV writer, and the `median()` template.

**Key design details**:
- `alignas(64)` on the atomic state prevents false sharing across cache lines.
- `REALTIME_PRIORITY_CLASS` for minimal jitter.
- `std::barrier(2)` ensures both threads have set their affinity before the ping-pong begins.
- Affinity is verified with a `GetCurrentProcessorNumberEx` retry loop — no blind `Sleep`.
- `Utils::median()` (via `std::nth_element`) is used instead of mean for robustness.
- Core topology (P-core vs E-core efficiency class) is displayed before the matrix.
- `CoreInfo` struct maps each flat core ID to (processor group, number, efficiency class).

## CLI

```
coreLatency.exe [--iters <N>] [--samples <N>] [--warmup <N>] [--output <file.csv>]
```

- `--iters` — CAS round-trips per sample (default 1000)
- `--samples` — number of timed batches per core pair (default 300)
- `--warmup` — warmup CAS iterations before timing starts (default: same as `--iters`)
- `--output` — write results to CSV (overwrites if exists)
