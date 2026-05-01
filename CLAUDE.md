# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projects

The solution (`coreLatency.sln`) contains two C++20 console applications:

| Project | What it measures | Technique |
|---------|-----------------|-----------|
| `coreLatency` | Inter-core CPU latency | Two-thread CAS ping-pong |
| `memLatency` | Memory hierarchy latency | Single-thread pointer-chasing |

Both projects share `src/utils.cpp` / `src/utils.h` (core topology, affinity, CPU name, priority, CSV, median). Both use Boost 1.80.0 program_options for CLI, MSVC v145 (VS 2026).

## Build

```bash
# From a VS 2026 x64 Native Tools command prompt:
.\nuget.exe restore coreLatency.sln
msbuild coreLatency.sln /p:Configuration=Release /p:Platform=x64
```

## Architecture

### coreLatency — inter-core latency

Two threads pinned to specific cores via `SetThreadGroupAffinity` (supports >64 cores via processor groups). They bounce an `alignas(64)` atomic `State` enum (PING/PONG) via `compare_exchange_weak(memory_order_relaxed)`. Each round-trip batch is timed with `std::chrono::high_resolution_clock`. Result is one-way latency: `total_time / 2 / iters`. Median across all samples (`std::nth_element`) is reported — robust against OS interrupt outliers. Output is a lower-triangular matrix printed to stdout, optionally CSV.

**Files**: `src/main.cpp` (CLI entry, iterates core pairs), `src/bench.h` / `src/bench.cpp` (ping-pong orchestration), `src/utils.h` / `src/utils.cpp` (shared utilities).

**Key details**:
- `alignas(64)` on the atomic state prevents false sharing.
- `REALTIME_PRIORITY_CLASS` for minimal jitter.
- `std::barrier(2)` ensures both threads set affinity before ping-pong begins.
- Affinity verified with a `GetCurrentProcessorNumberEx` retry loop — no blind `Sleep`.
- Core topology (P-core vs E-core efficiency class) displayed before the matrix.
- `CoreInfo` struct maps each flat core ID to (processor group, number, efficiency class).

### memLatency — memory hierarchy latency

Single-thread pointer-chasing through a linked list embedded in a buffer. The buffer size is stepped through powers of 2 from 512 bytes to a configurable max (default 256 MB), exposing L1/L2/L3/DRAM latency tiers. Stride defaults to 512 bytes (one pointer per cache line). Each measurement runs on a specific core set via the shared `Utils::setAffinity()`. Results stream to stdout/CSV as each (core, size) completes.

**Files**: `memLatency/main.cpp` (CLI, core/size iteration, streaming output), `memLatency/memBench.h` / `memLatency/memBench.cpp` (buffer generation, pointer-chasing measurement).

### tools/chart.html — CSV visualizer

Self-contained HTML page with CSV drag-and-drop (Papa Parse), Plotly.js rendering, and a custom CPU-grouped HTML legend. One trace per (cpu, core) with hash-based hue assignment (djb2) for color spread across CPUs. Per-CPU dash/symbol styling. Custom HTML tooltip overlay positioned at the cursor (Plotly SVG annotations can't render HTML). Legend supports per-dot and per-CPU-label hover highlighting, plus click-to-toggle visibility of individual cores or entire CPU groups. PNG export (`Plotly.toImage`) composites the chart + HTML legend onto a canvas with manual text/circle drawing.

Vendored: `tools/vendor/plotly-latest.min.js`, `tools/vendor/papaparse.min.js`.
CSV columns expected: `cpu`, `core`, `size(KB)`, `ns`.

## CLI

```
coreLatency.exe [--iters N] [--samples N] [--warmup N] [--output file.csv]
memLatency.exe [--iters N] [--samples N] [--warmup N] [--max-size KB] [--core N] [--list-sizes] [--output file.csv]
```

| Flag | coreLatency default | memLatency default | Description |
|------|---------------------|---------------------|-------------|
| `--iters` | 1000 | 100 | Operations per sample |
| `--samples` | 300 | 100 | Samples per (core, size); median reported |
| `--warmup` | same as iters | same as iters | Warmup iterations before timing |
| `--max-size` | — | 262144 (256 MB) | Max buffer size in KB |
| `--core` | — | all cores | Pin to a single core |
| `--list-sizes` | — | — | Print valid buffer sizes and exit |
| `--output` | — | — | Write CSV (overwrites if exists) |
