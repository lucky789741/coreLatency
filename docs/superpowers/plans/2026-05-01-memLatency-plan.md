# memLatency Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a console app that measures memory hierarchy latency (L1/L2/L3/DRAM) per core via random pointer-chasing and outputs a CSV of buffer size (KB) vs per-core latency (ns).

**Architecture:** New `memLatency` project in existing `coreLatency.sln`. Two new source files (`main.cpp`, `memBench.h/.cpp`) share `utils.{h,cpp}` from the existing `src/` directory via file references. CLI uses Boost program_options matching coreLatency's pattern. Memory measurement uses `VirtualAlloc`, Fisher-Yates shuffled pointer chains, and unrolled load loops.

**Tech Stack:** C++20, MSVC v145 (VS 2026), Boost 1.80.0 program_options, Windows APIs (VirtualAlloc, SetThreadGroupAffinity, GetLogicalProcessorInformation)

---

### Task 1: Create memLatency project structure

**Files:**
- Create: `memLatency\memLatency.vcxproj`
- Create: `memLatency\memLatency.vcxproj.filters`
- Create: `memLatency\packages.config`
- Modify: `coreLatency.sln`

- [ ] **Step 1: Create the project directory**

```powershell
New-Item -ItemType Directory -Path "C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency" -Force
```

- [ ] **Step 2: Create memLatency.vcxproj**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\memLatency.vcxproj`

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|Win32">
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|Win32">
      <Configuration>Release</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>16.0</VCProjectVersion>
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>{C1D2E3F4-A5B6-7890-CDEF-012345678902}</ProjectGuid>
    <RootNamespace>memLatency</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
    <ProjectName>memLatency</ProjectName>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v145</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <IncludePath>$(SolutionDir)packages\;$(IncludePath)</IncludePath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <IncludePath>$(SolutionDir)packages\;$(IncludePath)</IncludePath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <IncludePath>$(SolutionDir)packages\;$(IncludePath)</IncludePath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <IncludePath>$(SolutionDir)packages\;$(IncludePath)</IncludePath>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
      <Optimization>MaxSpeed</Optimization>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>
      <InlineFunctionExpansion>AnySuitable</InlineFunctionExpansion>
      <EnableFiberSafeOptimizations>false</EnableFiberSafeOptimizations>
      <AdditionalOptions>/favor:blend %(AdditionalOptions)</AdditionalOptions>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <None Include="packages.config" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="memBench.h" />
    <ClInclude Include="..\src\utils.h" />
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="main.cpp" />
    <ClCompile Include="memBench.cpp" />
    <ClCompile Include="..\src\utils.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
    <Import Project="..\packages\boost.1.80.0\build\boost.targets" Condition="Exists('..\packages\boost.1.80.0\build\boost.targets')" />
    <Import Project="..\packages\boost_program_options-vc143.1.80.0\build\boost_program_options-vc143.targets" Condition="Exists('..\packages\boost_program_options-vc143.1.80.0\build\boost_program_options-vc143.targets')" />
  </ImportGroup>
  <Target Name="EnsureNuGetPackageBuildImports" BeforeTargets="PrepareForBuild">
    <PropertyGroup>
      <ErrorText>此專案參考這部電腦上所缺少的 NuGet 套件。請啟用 NuGet 套件還原，以下載該套件。如需詳細資訊，請參閱 http://go.microsoft.com/fwlink/?LinkID=322105。缺少的檔案是 {0}。</ErrorText>
    </PropertyGroup>
    <Error Condition="!Exists('..\packages\boost.1.80.0\build\boost.targets')" Text="$([System.String]::Format('$(ErrorText)', '..\packages\boost.1.80.0\build\boost.targets'))" />
    <Error Condition="!Exists('..\packages\boost_program_options-vc143.1.80.0\build\boost_program_options-vc143.targets')" Text="$([System.String]::Format('$(ErrorText)', '..\packages\boost_program_options-vc143.1.80.0\build\boost_program_options-vc143.targets'))" />
  </Target>
</Project>
```

- [ ] **Step 3: Create memLatency.vcxproj.filters**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\memLatency.vcxproj.filters`

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="來源檔案">
      <UniqueIdentifier>{4FC737F1-C7A5-4376-A066-2A32D752A2FF}</UniqueIdentifier>
      <Extensions>cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx</Extensions>
    </Filter>
    <Filter Include="標頭檔">
      <UniqueIdentifier>{93995380-89BD-4b04-88EB-625FBE52EBFB}</UniqueIdentifier>
      <Extensions>h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd</Extensions>
    </Filter>
    <Filter Include="資源檔">
      <UniqueIdentifier>{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}</UniqueIdentifier>
      <Extensions>rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms</Extensions>
    </Filter>
  </ItemGroup>
  <ItemGroup>
    <None Include="packages.config" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="memBench.h">
      <Filter>標頭檔</Filter>
    </ClInclude>
    <ClInclude Include="..\src\utils.h">
      <Filter>標頭檔</Filter>
    </ClInclude>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="main.cpp">
      <Filter>來源檔案</Filter>
    </ClCompile>
    <ClCompile Include="memBench.cpp">
      <Filter>來源檔案</Filter>
    </ClCompile>
    <ClCompile Include="..\src\utils.cpp">
      <Filter>來源檔案</Filter>
    </ClCompile>
  </ItemGroup>
</Project>
```

- [ ] **Step 4: Create packages.config**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\packages.config`

```xml
<?xml version="1.0" encoding="utf-8"?>
<packages>
  <package id="boost" version="1.80.0" targetFramework="native" />
  <package id="boost_program_options-vc143" version="1.80.0" targetFramework="native" />
</packages>
```

- [ ] **Step 5: Modify coreLatency.sln to add the new project**

Add the new project entry after the existing project line. Replace the file content:

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\coreLatency.sln`

Old content:
```

Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.3.32804.467
MinimumVisualStudioVersion = 10.0.40219.1
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "coreLatencyIUC", "coreLatencyIUC.vcxproj", "{AB0B9A6C-4CC9-4F14-A639-D414A929A2F5}"
EndProject
Global
```

New content:
```

Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.3.32804.467
MinimumVisualStudioVersion = 10.0.40219.1
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "memLatency", "memLatency\memLatency.vcxproj", "{C1D2E3F4-A5B6-7890-CDEF-012345678902}"
EndProject
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "coreLatencyIUC", "coreLatencyIUC.vcxproj", "{AB0B9A6C-4CC9-4F14-A639-D414A929A2F5}"
EndProject
Global
```

And add the new project's configuration mappings in `GlobalSection(ProjectConfigurationPlatforms)`:

```
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Debug|x64.ActiveCfg = Debug|x64
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Debug|x64.Build.0 = Debug|x64
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Debug|x86.ActiveCfg = Debug|Win32
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Debug|x86.Build.0 = Debug|Win32
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Release|x64.ActiveCfg = Release|x64
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Release|x64.Build.0 = Release|x64
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Release|x86.ActiveCfg = Release|Win32
{C1D2E3F4-A5B6-7890-CDEF-012345678902}.Release|x86.Build.0 = Release|Win32
```

- [ ] **Step 6: Commit**

```bash
git add memLatency/ coreLatency.sln
git commit -m "feat: add memLatency project skeleton"
```

---

### Task 2: Create memBench.h

**Files:**
- Create: `memLatency\memBench.h`

- [ ] **Step 1: Write memBench.h**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\memBench.h`

```cpp
#pragma once
#include <Windows.h>
#include <vector>

#define STRIDE_DEFAULT  (512 / sizeof(char*))

namespace MemBench
{
    DWORD getCacheLineSize();
    size_t step(size_t k);
    std::vector<size_t> generateValidSizes(size_t maxSizeKB);
    DOUBLE measure(size_t bufferSize, DWORD stride, DWORD64 iters,
                   DWORD64 warmupIters, DWORD64 samples);
}
```

- [ ] **Step 2: Commit**

```bash
git add memLatency/memBench.h
git commit -m "feat: add memBench.h interface"
```

---

### Task 3: Create memBench.cpp

**Files:**
- Create: `memLatency\memBench.cpp`

- [ ] **Step 1: Write memBench.cpp with getCacheLineSize**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\memBench.cpp`

```cpp
#include "memBench.h"
#include "..\src\utils.h"
#include <chrono>
#include <random>
#include <stdexcept>

namespace MemBench
{

DWORD getCacheLineSize()
{
    DWORD size = 0;
    GetLogicalProcessorInformation(nullptr, &size);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(
        size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(buf.data(), &size))
        return STRIDE_DEFAULT;

    for (const auto& info : buf)
    {
        if (info.Relationship == RelationCache &&
            info.Cache.Level == 1)
        {
            return info.Cache.LineSize;
        }
    }
    return STRIDE_DEFAULT;
}

// Ported from lmbench lat_mem_rd.c
size_t step(size_t k)
{
    if (k < 1024) {
        k = k * 2;
    } else if (k < 4 * 1024) {
        k += 1024;
    } else {
        size_t s;
        for (s = 32 * 1024; s <= k; s *= 2)
            ;
        k += s / 16;
    }
    return k;
}

std::vector<size_t> generateValidSizes(size_t maxSizeKB)
{
    std::vector<size_t> sizes;
    size_t maxBytes = maxSizeKB * 1024ULL;
    size_t k = 512;
    while (k <= maxBytes) {
        sizes.push_back(k);
        k = step(k);
    }
    return sizes;
}

static void buildChain(char* buf, size_t bufSize, DWORD stride)
{
    size_t N = bufSize / stride;
    if (N < 2) return;

    std::vector<size_t> indices(N);
    for (size_t i = 0; i < N; i++)
        indices[i] = i;

    // Fisher-Yates shuffle
    static thread_local std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    for (size_t i = N - 1; i > 0; i--) {
        size_t j = rng() % (i + 1);
        std::swap(indices[i], indices[j]);
    }

    for (size_t i = 0; i < N; i++) {
        size_t next = indices[(i + 1) % N];
        *(char**)(buf + i * stride) = buf + next * stride;
    }
}

#define ONE   p = (char**)*p;
#define FIVE  ONE ONE ONE ONE ONE
#define TEN   FIVE FIVE

__declspec(noinline) static void escape(void*) {}

DOUBLE measure(size_t bufferSize, DWORD stride, DWORD64 iters,
               DWORD64 warmupIters, DWORD64 samples)
{
    size_t slotCount = bufferSize / stride;
    if (slotCount < 2)
        slotCount = 2;
    size_t adjustedSize = slotCount * stride;

    char* buf = (char*)VirtualAlloc(nullptr, adjustedSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buf)
        throw std::bad_alloc();

    std::vector<DOUBLE> durations;
    durations.reserve(samples);

    for (DWORD64 s = 0; s < samples; s++) {
        buildChain(buf, adjustedSize, stride);
        char* p = buf;

        // Warmup: full-chain traversals
        DWORD64 warmupLoads = warmupIters;
        while (warmupLoads >= 10) {
            TEN;
            warmupLoads -= 10;
        }
        while (warmupLoads-- > 0) ONE;

        // Timed traversal
        DWORD64 remaining = iters;
        auto start = std::chrono::high_resolution_clock::now();
        while (remaining >= 10) {
            TEN;
            remaining -= 10;
        }
        while (remaining-- > 0) ONE;
        auto end = std::chrono::high_resolution_clock::now();

        DOUBLE duration = (DOUBLE)(end - start).count() / iters;
        durations.push_back(duration);

        escape((void*)p);
    }

    VirtualFree(buf, 0, MEM_RELEASE);
    return Utils::median(durations);
}

} // namespace MemBench
```

- [ ] **Step 2: Commit**

```bash
git add memLatency/memBench.cpp
git commit -m "feat: add memBench.cpp with pointer-chasing measurement"
```

---

### Task 4: Create main.cpp

**Files:**
- Create: `memLatency\main.cpp`

- [ ] **Step 1: Write main.cpp**

File: `C:\Users\Master\source\repos\lucky789741\coreLatency\memLatency\main.cpp`

```cpp
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <boost/program_options.hpp>
#include "..\src\utils.h"
#include "memBench.h"

namespace po = boost::program_options;

static void printValidSizes(size_t maxSizeKB = 262144)
{
    auto sizes = MemBench::generateValidSizes(maxSizeKB);
    std::cout << "Valid buffer sizes (KB):\n";
    for (auto b : sizes) {
        DOUBLE kb = (DOUBLE)b / 1024.0;
        std::cout << std::fixed << std::setprecision(1) << kb << '\n';
    }
}

static bool isValidMaxSize(size_t sizeKB)
{
    auto sizes = MemBench::generateValidSizes(sizeKB);
    return !sizes.empty() && sizes.back() == sizeKB * 1024ULL;
}

static void printNearestSizes(size_t sizeKB)
{
    // Generate a broad set and find nearest
    auto sizes = MemBench::generateValidSizes(1048576); // up to 1 TB
    std::vector<DOUBLE> sizesKB;
    for (auto b : sizes)
        sizesKB.push_back((DOUBLE)b / 1024.0);

    DOUBLE target = (DOUBLE)sizeKB;
    size_t closestIdx = 0;
    DOUBLE closestDiff = std::abs(sizesKB[0] - target);
    for (size_t i = 1; i < sizesKB.size(); i++) {
        DOUBLE diff = std::abs(sizesKB[i] - target);
        if (diff < closestDiff) {
            closestDiff = diff;
            closestIdx = i;
        }
    }

    std::cerr << "Nearest valid sizes (KB):\n";
    if (closestIdx > 0)
        std::cerr << "  " << sizesKB[closestIdx - 1] << '\n';
    std::cerr << "  " << sizesKB[closestIdx] << '\n';
    if (closestIdx + 1 < sizesKB.size())
        std::cerr << "  " << sizesKB[closestIdx + 1] << '\n';
}

int main(INT argc, CHAR* argv[])
{
    try
    {
        DWORD64 iters = 1000;
        DWORD64 samples = 300;
        DWORD64 warmupIters = 0; // 0 means same as iters
        size_t maxSizeKB = 262144;
        std::string outputFile;
        bool listSizes = false;

        po::options_description desc("Available options");
        desc.add_options()
            ("help", "Print this message")
            ("iters", po::value<DWORD64>(), "Pointer loads per sample (default: 1000)")
            ("samples", po::value<DWORD64>(), "Timed batches per core per buffer size (default: 300)")
            ("warmup", po::value<DWORD64>(), "Warmup loads before timing (default: same as --iters)")
            ("max-size", po::value<size_t>(), "Max buffer size in KB, must be a valid log-scale step (default: 262144)")
            ("list-sizes", po::bool_switch(&listSizes), "Print valid buffer sizes in KB and exit")
            ("output", po::value<std::string>(), "Save CSV to file (overwrites if exists)")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            std::cout << "All buffer sizes are in KB (kilobytes).\n";
            return 0;
        }

        if (vm.count("iters"))
            iters = vm["iters"].as<DWORD64>();
        if (vm.count("samples"))
            samples = vm["samples"].as<DWORD64>();
        if (vm.count("warmup"))
            warmupIters = vm["warmup"].as<DWORD64>();
        else
            warmupIters = iters;
        if (vm.count("max-size")) {
            maxSizeKB = vm["max-size"].as<size_t>();
        }
        if (vm.count("output"))
            outputFile = vm["output"].as<std::string>();

        // --list-sizes: print all valid buffer sizes and exit
        if (listSizes) {
            printValidSizes(1048576); // up to 1 TB for listing
            return 0;
        }

        // Validate --max-size
        if (!isValidMaxSize(maxSizeKB)) {
            std::cerr << "Error: " << maxSizeKB
                      << " KB is not a valid log-scale buffer size.\n";
            printNearestSizes(maxSizeKB);
            return 1;
        }

        // Init topology and detect cache line
        Utils::initCoreTopology();
        Utils::setHighestPriority();
        DWORD cpuCount = Utils::getCPUCount();
        DWORD cacheLine = MemBench::getCacheLineSize();

        // Generate valid buffer sizes in bytes
        auto validSizes = MemBench::generateValidSizes(maxSizeKB);

        // Print header info
        std::cout << "CPU: " << Utils::getCPUName() << std::endl;
        std::cout << "Cores: " << cpuCount << std::endl;
        std::cout << "Cache line: " << cacheLine << " bytes" << std::endl;
        std::cout << "Iters: " << iters << ", Samples: " << samples
                  << ", Warmup: " << warmupIters << std::endl;
        std::cout << "Max buffer size (KB): " << maxSizeKB << '\n' << std::endl;

        const auto& topo = Utils::getCoreTopology();
        std::cout << "Core topology (E=E-core, P=P-core): ";
        for (DWORD i = 0; i < cpuCount; i++)
            std::cout << i << '(' << (topo[i].efficiencyClass == 1 ? 'E' : 'P') << ") ";
        std::cout << "\n" << std::endl;

        // CSV header
        std::cout << "size(KB)";
        for (DWORD c = 0; c < cpuCount; c++)
            std::cout << ',' << 'c' << c << '(' << (topo[c].efficiencyClass == 1 ? 'E' : 'P') << ')';
        std::cout << '\n';

        // Per-core per-size measurement
        std::vector<std::vector<DOUBLE>> results;
        for (const auto& sizeBytes : validSizes) {
            DOUBLE sizeKB = (DOUBLE)sizeBytes / 1024.0;
            std::vector<DOUBLE> row;
            row.reserve(cpuCount);

            for (DWORD core = 0; core < cpuCount; core++) {
                Utils::setAffinity(core);
                DOUBLE lat = MemBench::measure(sizeBytes, cacheLine,
                                               iters, warmupIters, samples);
                row.push_back(lat);
            }

            results.push_back(row);
        }

        // Print CSV body
        auto csvOut = [&](std::ostream& os) {
            for (size_t r = 0; r < validSizes.size(); r++) {
                DOUBLE sizeKB = (DOUBLE)validSizes[r] / 1024.0;
                os << std::fixed << std::setprecision(1) << sizeKB;
                for (DWORD c = 0; c < cpuCount; c++) {
                    os << ',' << std::fixed << std::setprecision(1)
                       << results[r][c];
                }
                os << '\n';
            }
        };

        csvOut(std::cout);

        if (!outputFile.empty()) {
            std::ofstream fs(outputFile, std::ios::out | std::ios::trunc);
            if (!fs.is_open())
                throw std::runtime_error("Unable to write to: " + outputFile);
            csvOut(fs);
            std::cerr << "Results saved to " << outputFile << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add memLatency/main.cpp
git commit -m "feat: add memLatency CLI with core iteration and CSV output"
```

---

### Task 5: Build and verify

- [ ] **Step 1: Restore NuGet packages and build Release x64**

From a VS 2026 x64 Native Tools command prompt:

```bash
cd C:\Users\Master\source\repos\lucky789741\coreLatency
.\nuget.exe restore coreLatency.sln
msbuild coreLatency.sln /p:Configuration=Release /p:Platform=x64
```

Expected: Build succeeds with no errors. Output: `x64\Release\memLatency.exe`.

- [ ] **Step 2: Verify --help**

```powershell
.\x64\Release\memLatency.exe --help
```

Expected: Prints options including `--list-sizes`, `--max-size`, with "All buffer sizes are in KB" note.

- [ ] **Step 3: Verify --list-sizes**

```powershell
.\x64\Release\memLatency.exe --list-sizes
```

Expected: Prints ~50 buffer sizes in KB, starting from 0.5, doubling through ~4.0, then logarithmic to 262144.0 (or 1048576.0 if listing up to 1 TB). No measurement runs.

- [ ] **Step 4: Verify --max-size validation rejects invalid value**

```powershell
.\x64\Release\memLatency.exe --max-size 250000
```

Expected: Error message "250000 KB is not a valid log-scale buffer size" with nearest valid sizes (e.g., 229376.0, 262144.0).

- [ ] **Step 5: Verify --max-size accepts valid value**

```powershell
.\x64\Release\memLatency.exe --max-size 262144 --samples 3 --iters 100
```

Expected: No validation error, runs measurement, prints CSV to stdout. Quick test with small samples to verify the pipeline works end-to-end.

- [ ] **Step 6: Commit if any fixes were needed**

Only if changes were made during verification:

```bash
git add -A
git commit -m "fix: build verification fixes"
```
