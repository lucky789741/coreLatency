# memLatency Design Spec

## Overview

Measure memory hierarchy latency (L1/L2/L3/DRAM) per core. A pointer-chasing chain with randomized slots walks a buffer that grows from 512 bytes to a configurable maximum (default 262144 KB) on a logarithmic scale. Each core runs independently — a thread is pinned to the target core, a fresh buffer is allocated (first-touch on that core), and the median traversal time per pointer load is recorded. The result is a CSV table of buffer size (KB) vs per-core latency (ns), showing cache-layer transitions and any core-to-core variation.

## Architecture

```
coreLatency.sln
├── coreLatency/          (existing — unchanged)
│   ├── main.cpp
│   ├── bench.h / bench.cpp
│   └── utils.h / utils.cpp
└── memLatency/           (new project, console app x64)
    ├── main.cpp          — CLI, step(), core iteration, output
    ├── memBench.h / .cpp — random pointer-chasing, timing
    └── → links utils.{h,cpp} via file reference in .vcxproj
```

Utils is shared at the source-file level (add existing file as link in the new .vcxproj). Both projects compile the same utils.{h,cpp}.

## Measurement Algorithm

### Pointer-chasing chain (random access)

1. Allocate `bufferSize` bytes, page-aligned (`VirtualAlloc`).
2. Slot size = cache line size (from `GetLogicalProcessorInformation`, typically 64 bytes).
3. Number of slots N = `bufferSize / cacheLineSize`.
4. Create an index array `[0, 1, ..., N-1]` and Fisher-Yates shuffle it.
5. In each slot `i`, write a pointer to the start address of slot `shuffled[(i+1) % N]`:
   ```
   for (i = 0; i < N; i++)
       *(char**)(buf + i * stride) = buf + shuffled[(i+1) % N] * stride;
   ```
6. The chain visits every slot exactly once per cycle in random order, defeating hardware prefetchers.

### Timing

- **Warmup**: traverse the full chain without timing to establish cache state.
- **Sample**: time N pointer loads (`p = *(char**)p`), compute per-load latency.
- **Samples**: repeat `samples` times, take the median.
- Load loop unrolled (100 loads per iteration body) to reduce loop-overhead bias, same pattern as lmbench's `HUNDRED` macro.

### Per-core flow

```
for each core:
    set thread affinity to core
    for each bufferSize in validSizes:
        alloc buffer (first-touch on this core → local NUMA)
        build shuffled chain
        warmup traverse
        for each sample:
            time traverse
        save median latency
        free buffer
```

Buffer is re-allocated per core to ensure first-touch happens on the pinned core. No explicit NUMA APIs — Windows first-touch policy gives local-node pages.

## size_t step(size_t k)

Ported from lmbench `lat_mem_rd.c`:

```cpp
size_t step(size_t k) {
    if (k < 1024)           k = k * 2;
    else if (k < 4 * 1024)  k += 1024;
    else {
        size_t s;
        for (s = 32 * 1024; s <= k; s *= 2);
        k += s / 16;
    }
    return k;
}
```

Start: 512 (bytes). Generates ~40 logarithmically-spaced sizes to 262144 KB default max.

## CLI

```
memLatency.exe [--iters <N>] [--samples <N>] [--warmup <N>]
               [--max-size <KB>] [--list-sizes] [--output <file.csv>]
```

| Option | Default | Description |
|--------|---------|-------------|
| `--iters` | 1000 | Pointer loads per sample |
| `--samples` | 300 | Timed batches per core per buffer size |
| `--warmup` | =iters | Warmup traversals before timing |
| `--max-size <KB>` | 262144 | Override max buffer size (must be a valid step size) |
| `--list-sizes` | — | Print all valid buffer sizes in KB and exit |
| `--output` | — | Write CSV to file |

### --max-size validation

1. Generate `validSizes` by calling `step()` from 512 up to but not exceeding 1 TB.
2. If the user-supplied value is not in `validSizes`, print an error listing the nearest valid sizes and exit.
3. All buffer-size inputs and outputs are in KB (kilobytes).

### --list-sizes

Prints `validSizes` (KB, with decimal for values < 1 KB), then exits immediately. No measurement runs.

## Output Format

CSV, both to stdout and `--output` file:

```
size(KB),core0,core1,core2,...
0.5,1.2,1.3,1.2,...
1.0,1.3,1.4,1.3,...
2.0,1.4,1.5,1.4,...
...
262144.0,110.5,112.3,108.9,...
```

- First column: buffer size in KB (with one decimal for sub-KB sizes).
- Remaining columns: per-core median latency in nanoseconds (one decimal).
- Core column headers include efficiency class: `core0(P)`, `core1(P)`, `core2(E)`.
- Preceding stdout lines show CPU name, core count, cache line size, and parameters.

## Dependencies

- Windows APIs: `VirtualAlloc`, `SetThreadGroupAffinity`, `GetLogicalProcessorInformation`, `GetSystemCpuSetInformation`
- Boost 1.80.0: `program_options` (same pattern as coreLatency)
- C++20, MSVC v145, x64

## Error Handling

- Invalid `--max-size`: print valid neighbors, exit.
- Unknown CLI options: Boost handles with usage message.
- Affinity failure: throw with error code.
- Allocation failure: skip that size and warn, or exit if repeated.
