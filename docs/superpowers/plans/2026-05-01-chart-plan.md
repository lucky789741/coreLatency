# Chart Visualizer & CSV Update Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `cpu` column to memLatency CSV output and build a standalone HTML chart tool with drag-and-drop CSV loading, interactive Plotly curves, and PNG export.

**Architecture:** Two independent changes: (1) memLatency `main.cpp` gets a `cpu` column prepended to CSV output, (2) a new `tools/chart.html` with vendored Plotly+PapaParse renders multi-file memory latency curves. No build step — chart.html opens directly in a browser.

**Tech Stack:** C++20 for csv update; vanilla HTML/CSS/JS + Plotly.js + Papa Parse for chart tool.

---

### Task 1: Update memLatency CSV to include cpu column

**Files:**
- Modify: `memLatency\main.cpp`

- [ ] **Step 1: Read current code around CSV output**

The changes are at lines 151-163 (CSV headers) and lines 181-189 (CSV data rows).

- [ ] **Step 2: Add cpu column to CSV header and metadata output**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\main.cpp`

Change the CSV header from `"core,size(KB),ns"` to `"cpu,core,size(KB),ns"`.

Edit at line 152:
```
std::cout << "cpu,core,size(KB),ns\n";
```

Edit at line 163:
```
fs << "cpu,core,size(KB),ns\n";
```

- [ ] **Step 3: Add cpu name to streaming output rows**

Edit at lines 181-183, add cpu name before core:
```cpp
std::cout << Utils::getCPUName() << ','
          << core << ','
          << std::fixed << std::setprecision(1) << sizeKB << ','
          << std::fixed << std::setprecision(1) << lat << '\n';
```

Edit at lines 185-188, same for file output:
```cpp
fs << Utils::getCPUName() << ','
   << core << ','
   << std::fixed << std::setprecision(1) << sizeKB << ','
   << std::fixed << std::setprecision(1) << lat << '\n';
```

- [ ] **Step 4: Build and verify CSV output**

From a VS 2026 x64 Native Tools command prompt:

```powershell
msbuild coreLatency.sln /p:Configuration=Release /p:Platform=x64
```

Expected: Build succeeds, 0 errors.

Test:
```powershell
.\x64\Release\memLatency.exe --core 0 --max-size 64 --samples 3 --iters 10
```

Expected: CSV header is `cpu,core,size(KB),ns` and each row starts with `12th Gen Intel(R) Core(TM) i5-1235U,0,...`.

- [ ] **Step 5: Commit**

```bash
git add memLatency/main.cpp
git commit -m "feat: add cpu column to memLatency CSV output"
```

---

### Task 2: Download vendored JS libraries

**Files:**
- Create: `tools\vendor\plotly-latest.min.js`
- Create: `tools\vendor\papaparse.min.js`

- [ ] **Step 1: Create directories**

```powershell
New-Item -ItemType Directory -Path "C:\Users\Master\source\repos\lucky789741\coreLatency\tools\vendor" -Force
```

- [ ] **Step 2: Download Plotly.js**

```powershell
Invoke-WebRequest -Uri "https://cdn.plot.ly/plotly-latest.min.js" -OutFile "C:\Users\Master\source\repos\lucky789741\coreLatency\tools\vendor\plotly-latest.min.js"
```

- [ ] **Step 3: Download Papa Parse**

```powershell
Invoke-WebRequest -Uri "https://cdn.jsdelivr.net/npm/papaparse@5.4.1/papaparse.min.js" -OutFile "C:\Users\Master\source\repos\lucky789741\coreLatency\tools\vendor\papaparse.min.js"
```

- [ ] **Step 4: Verify downloads**

```powershell
Get-ChildItem "C:\Users\Master\source\repos\lucky789741\coreLatency\tools\vendor"
```

Expected: Two files present, each >100 KB.

- [ ] **Step 5: Commit**

```bash
git add tools/vendor/
git commit -m "chore: add vendored Plotly.js and Papa Parse for chart tool"
```

---

### Task 3: Create chart.html

**Files:**
- Create: `tools\chart.html`

- [ ] **Step 1: Write chart.html**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\tools\chart.html`

```html
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Memory Latency Chart</title>
<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #1a1a2e; color: #eee; min-height: 100vh; display: flex; flex-direction: column; }
  .toolbar { display: flex; gap: 8px; padding: 12px 16px; background: #16213e; border-bottom: 1px solid #0f3460; align-items: center; }
  .toolbar button { padding: 8px 16px; border: 1px solid #0f3460; border-radius: 4px; cursor: pointer; font-size: 14px; background: #0f3460; color: #eee; }
  .toolbar button:hover { background: #1a508b; }
  .toolbar .file-count { margin-left: auto; font-size: 13px; color: #888; }
  #drop { flex: 1; display: flex; align-items: center; justify-content: center; min-height: 300px; margin: 16px; border: 2px dashed #0f3460; border-radius: 8px; transition: border-color 0.2s, background 0.2s; }
  #drop.drag-over { border-color: #e94560; background: rgba(233,69,96,0.05); }
  #drop.has-data { min-height: 60px; margin: 8px 16px; }
  #drop p { color: #666; font-size: 18px; pointer-events: none; }
  #chart { flex: 1; min-height: 500px; display: none; margin: 0 16px 16px; }
  #chart.visible { display: block; }
  .toast { position: fixed; top: 16px; right: 16px; padding: 12px 20px; border-radius: 4px; font-size: 14px; z-index: 999; animation: fadeIn 0.3s; }
  .toast.error { background: #e94560; color: #fff; }
  .toast.info { background: #0f3460; color: #eee; }
  @keyframes fadeIn { from { opacity: 0; transform: translateY(-8px); } to { opacity: 1; transform: translateY(0); } }
</style>
</head>
<body>

<div class="toolbar">
  <button id="btnClear" onclick="clearAll()" disabled>Clear</button>
  <button id="btnExport" onclick="exportPNG()" disabled>Export PNG</button>
  <span class="file-count" id="fileCount"></span>
</div>

<div id="drop"><p>Drop CSV files here</p></div>
<div id="chart"></div>

<script src="vendor/plotly-latest.min.js"></script>
<script src="vendor/papaparse.min.js"></script>
<script>
// --- State ---
let allTraces = [];       // { cpu, core, ec, filename, x:[], y:[] }
let fileNames = new Set();

// --- djb2 hash ---
function djb2(str) {
  let hash = 5381;
  for (let i = 0; i < str.length; i++) hash = ((hash << 5) + hash) + str.charCodeAt(i);
  return hash >>> 0;
}

// --- Color from CPU name + core index ---
function traceColor(cpu, coreIdx, coreCount) {
  let base = (djb2(cpu) % 360);
  let gap = Math.max(8, Math.floor(60 / Math.max(coreCount, 1)));
  let hue = (base + coreIdx * gap) % 360;
  return `hsl(${hue}, 70%, 50%)`;
}

// --- Drop zone ---
const drop = document.getElementById('drop');
const chartDiv = document.getElementById('chart');
const btnClear = document.getElementById('btnClear');
const btnExport = document.getElementById('btnExport');
const fileCount = document.getElementById('fileCount');

drop.addEventListener('dragover', e => { e.preventDefault(); drop.classList.add('drag-over'); });
drop.addEventListener('dragleave', () => drop.classList.remove('drag-over'));
drop.addEventListener('drop', e => {
  e.preventDefault();
  drop.classList.remove('drag-over');
  const files = Array.from(e.dataTransfer.files).filter(f => f.name.endsWith('.csv'));
  if (files.length === 0) { toast('No CSV files found', 'error'); return; }
  files.forEach(f => parseCSV(f));
});

// --- CSV parsing ---
function parseCSV(file) {
  fileNames.add(file.name);
  Papa.parse(file, {
    header: true,
    skipEmptyLines: true,
    dynamicTyping: true,
    complete: function(results) {
      if (results.errors.length > 0) {
        toast(`Parse error in ${file.name}: ${results.errors[0].message}`, 'error');
        return;
      }
      if (!results.meta.fields.includes('cpu') || !results.meta.fields.includes('core') ||
          !results.meta.fields.includes('size(KB)') || !results.meta.fields.includes('ns')) {
        toast(`Missing required columns in ${file.name} (need: cpu,core,size(KB),ns)`, 'error');
        return;
      }

      // Group rows by (cpu, core)
      let groups = {};
      results.data.forEach(row => {
        if (row.cpu == null || row.core == null || row['size(KB)'] == null || row.ns == null) return;
        let key = row.cpu + '|' + row.core;
        if (!groups[key]) groups[key] = { cpu: row.cpu, core: row.core, x: [], y: [] };
        groups[key].x.push(row['size(KB)']);
        groups[key].y.push(row.ns);
      });

      // Compute efficiency class per file+core from data (not stored in CSV, default to '-')
      // We don't have EC in the CSV, mark as '?' if unknown
      let ec = '?';

      // Sort groups by core
      let entries = Object.values(groups).sort((a, b) => a.core - b.core);
      let coreCount = entries.length;

      entries.forEach(g => {
        allTraces.push({
          cpu: g.cpu,
          core: g.core,
          ec: ec,
          filename: file.name,
          x: g.x,
          y: g.y,
          color: traceColor(g.cpu, g.core, coreCount),
        });
      });

      render();
      toast(`Loaded ${file.name} (${entries.length} cores)`, 'info');
    },
    error: function(err) {
      toast(`Failed to read ${file.name}: ${err.message}`, 'error');
    }
  });
}

// --- Plotly render ---
function render() {
  if (allTraces.length === 0) return;

  let plotTraces = allTraces.map(t => ({
    x: t.x,
    y: t.y,
    type: 'scatter',
    mode: 'lines',
    name: t.cpu + ' / core' + t.core,
    line: { color: t.color, width: 2 },
    hovertemplate: `<b>${t.cpu}</b><br>core${t.core}  size=%{x:.1f} KB  ns=%{y:.1f}<extra></extra>`,
  }));

  let layout = {
    xaxis: { title: 'Buffer size (KB)', type: 'log', gridcolor: '#333', zeroline: false },
    yaxis: { title: 'Latency (ns)', gridcolor: '#333', zeroline: false },
    legend: { orientation: 'h', y: -0.2, font: { color: '#ccc', size: 11 } },
    plot_bgcolor: '#1a1a2e',
    paper_bgcolor: '#1a1a2e',
    font: { color: '#eee' },
    margin: { t: 20, r: 20, b: 100, l: 60 },
  };

  Plotly.react(chartDiv, plotTraces, layout, { responsive: true });

  chartDiv.classList.add('visible');
  drop.classList.add('has-data');
  btnClear.disabled = false;
  btnExport.disabled = false;
  fileCount.textContent = fileNames.size + ' file(s) loaded';
}

// --- Clear ---
function clearAll() {
  allTraces = [];
  fileNames.clear();
  Plotly.purge(chartDiv);
  chartDiv.classList.remove('visible');
  drop.classList.remove('has-data');
  btnClear.disabled = true;
  btnExport.disabled = false;  // keep enabled
  fileCount.textContent = '';
  toast('Cleared all data', 'info');
}

// --- Export PNG ---
function exportPNG() {
  if (allTraces.length === 0) return;
  Plotly.downloadImage(chartDiv, { format: 'png', width: 1920, height: 1080, filename: 'latency-chart' });
}

// --- Toast ---
function toast(msg, type) {
  let el = document.createElement('div');
  el.className = 'toast ' + type;
  el.textContent = msg;
  document.body.appendChild(el);
  setTimeout(() => el.remove(), 3000);
}
</script>
</body>
</html>
```

- [ ] **Step 2: Commit**

```bash
git add tools/chart.html
git commit -m "feat: add chart.html memory latency visualizer"
```

---

### Task 4: Build and verify end-to-end

- [ ] **Step 1: Build memLatency with updated CSV format**

```powershell
msbuild coreLatency.sln /p:Configuration=Release /p:Platform=x64
```

Expected: Build succeeds.

- [ ] **Step 2: Generate a test CSV**

```powershell
.\x64\Release\memLatency.exe --core 0 --max-size 65536 --samples 5 --iters 100 --output .\tools\test.csv
```

Expected: CSV file created at `tools\test.csv` with `cpu,core,size(KB),ns` header.

- [ ] **Step 3: Open chart.html in browser**

Open `tools\chart.html` in Chrome/Edge/Firefox. Drag `tools\test.csv` onto the drop zone.

Expected:
- Curves render (one per core tested).
- X-axis logarithmic, Y-axis latency in ns.
- Legend shows `CPU名称 / coreN`.
- Hover tooltip shows values.
- Legend click toggles curve visibility.
- PNG export button works.
- Clear button resets everything.

- [ ] **Step 4: Test multi-file scenario**

Generate a second CSV with different max-size:
```powershell
.\x64\Release\memLatency.exe --core 0 --max-size 32768 --samples 3 --iters 50 --output .\tools\test2.csv
```

Drag both `test.csv` and `test2.csv` onto chart.html.

Expected: Both files' curves visible with different color families. Legend distinguishes them.

- [ ] **Step 5: Clean up test files and commit if fixes needed**

```powershell
Remove-Item tools\test.csv, tools\test2.csv -ErrorAction SilentlyContinue
```

Only if any fixes were made during verification:
```bash
git add tools/chart.html
git commit -m "fix: chart.html verification fixes"
```
