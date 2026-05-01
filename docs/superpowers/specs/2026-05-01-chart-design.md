# Memory Latency Chart Visualizer Design Spec

## Overview

A zero-dependency single-file HTML chart tool that renders memory latency curves from memLatency CSV output. Drag-and-drop multiple CSV files (from different CPUs or benchmark runs), see interactive latency curves with per-core color coding, and export to PNG.

Also updates memLatency's CSV format to include a `cpu` column for self-contained CPU identification.

## memLatency CSV Format Update

### Current format

```
core,size(KB),ns
0,0.5,1.0
0,1.0,1.3
```

### New format

```
cpu,core,size(KB),ns
12th Gen Intel(R) Core(TM) i5-1235U,0,0.5,1.0
12th Gen Intel(R) Core(TM) i5-1235U,0,1.0,1.3
...
```

- First column = `cpu` — value from `Utils::getCPUName()`.
- Header info lines (CPU model, core count, cache line, etc.) remain as stdout echo but are NOT in the CSV body.
- `--output` file contains only the 4-column CSV body (no header lines).

### Changes required

- `memLatency/main.cpp`: update CSV header and output rows to include `cpu` column.
- No change to `memBench` or `Utils`.

## chart.html Architecture

Single static HTML file. No build step, no server, no dependencies beyond CDN-loaded Plotly.js and Papa Parse.

### Dependencies (CDN)

| Library | URL | Purpose |
|---------|-----|---------|
| Plotly.js | `cdn.plot.ly/plotly-latest.min.js` | Chart rendering, interactivity, PNG export |
| Papa Parse | `cdn.jsdelivr.net/npm/papaparse` | Robust CSV parsing |

### Structure

```
chart.html
├── <style>           — drop zone, buttons, layout
├── <div id="drop">   — drag-and-drop target area
├── <div id="chart">  — Plotly chart container
├── <button>          — Export PNG, Clear, etc.
├── <script src="...">— Plotly CDN
├── <script src="...">— Papa Parse CDN
└── <script>          — application logic
```

### Workflow

1. User opens `chart.html` in a browser.
2. User drags one or more CSV files onto the drop zone.
3. Each CSV is parsed via Papa Parse. The `cpu` column is extracted.
4. All cores from all files are rendered as separate curves on a shared Plotly chart.
5. User interacts: zoom, pan, hover, toggle legend entries, export PNG.
6. Dragging additional files appends new curves to the existing chart.

### Chart Configuration

- **X-axis**: `size(KB)`, logarithmic scale (`type: 'log'`).
- **Y-axis**: latency in nanoseconds (`ns`).
- **Traces**: one per core per file. Trace name = `"CPU名称 / coreN(EClass)"`.
- **Line width**: 2px default, 4px on legend hover (Plotly default behavior).
- **Grid**: white background with light grey gridlines.

### Color Strategy

Each CPU gets a distinct base hue. Cores within the same CPU get a small hue offset, keeping them in the same color family.

```
baseHue = djb2_hash(cpuName) % 360
gap     = max(8, 60 / maxCoresInFile)   // auto-scale gap so range ≤ 60°
hue     = (baseHue + coreIndex * gap) % 360
color   = HSL(hue, 70%, 50%)
```

- Different CPUs → well-separated base hues (hash distributes uniformly across 0-359).
- Same CPU cores → same color family (e.g., all greens or all purples), distinguishable by offset.
- Edge case: single-core CPUs get just the base hue.

### Legend

- Positioned below the chart (horizontal).
- Format: `"cpuName / coreN(ec)"`.
- Click to toggle visibility.
- Hover to highlight (Plotly `legenditemclick` + `plotly_hover` built-in).

### Export PNG

- Button labeled "Export PNG" positioned above the chart.
- Uses `Plotly.downloadImage(plotDiv, {format: 'png', width: 1920, height: 1080})`.
- Exports the current zoom/pan state (WYSIWYG).

### Additional Features

- **Clear button**: removes all traces, resets drop state.
- **Responsive**: chart fills available width, maintains 16:9 aspect ratio.
- **Empty state**: drop zone shows instructions ("Drop CSV files here") before any data is loaded.

### Error Handling

- Non-CSV files: silently ignored.
- CSV missing `cpu` column: error toast, file skipped.
- CSV with zero rows: silently ignored.
- Parse errors: error toast with filename.

## Testing

- Open `chart.html` in Chrome/Edge/Firefox.
- Drag a single CSV from `memLatency --output`, verify curves render.
- Drag a second CSV from a different run/CPU, verify both appear with distinct color families.
- Verify legend hover highlighting.
- Verify PNG export produces a readable image.
- Verify Clear button resets the chart.

## Scope

This spec covers two changes:

1. **memLatency CSV format** — add `cpu` column (small change to `main.cpp`).
2. **chart.html** — new standalone file in `tools/` directory, not part of the MSBuild solution.
