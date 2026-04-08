# Building and Testing microhash

This document covers how to build each implementation from source (debug and release), run the CLI tools, execute the test suites, run the benchmarks, and use the Docker image.

All paths are relative to the **repository root** unless stated otherwise.

---

## Prerequisites

| Tool | Minimum version | Purpose |
|------|----------------|---------|
| C++ compiler (GCC, Clang, MSVC) | C++17 | C++ implementation + tests |
| .NET SDK | 9.0 | C# implementation, tests, benchmarks |
| Docker *(optional)* | 20.10+ | Container build / run |

> **Note:** The C++ implementation (`src/cpp/microhash.hpp`) is a header-only library — no separate compilation step is needed to *use* it. The build steps below apply to the CLI tool (`main.cpp`) and the test suite (`tests/cpp/microhash_tests.cpp`).

---

## C++ Implementation

All commands are run from the **repository root** unless a `cd` is shown.

### Debug build — CLI tool

```sh
g++ -std=c++17 -g -O0 \
    -o microhash_debug \
    src/cpp/main.cpp
```

With Clang:
```sh
clang++ -std=c++17 -g -O0 \
    -o microhash_debug \
    src/cpp/main.cpp
```

With MSVC (Developer Command Prompt):
```cmd
cl /std:c++17 /Zi /Od /Fe:microhash_debug.exe src\cpp\main.cpp
```

### Release build — CLI tool

```sh
g++ -std=c++17 -O2 -DNDEBUG \
    -o microhash \
    src/cpp/main.cpp
```

### Running the CLI tool

Hash a string passed as an argument:
```sh
./microhash "Hello, World!"
# microhash("Hello, World!") = 0x352256EFEDC72BD1
```

Hash from standard input (interactive):
```sh
./microhash
# Enter a string to hash using MicroHash64:
# > abc
# microhash("abc") = 0x8874CA7BE18B8218
```

Print hashes for all built-in test vectors:
```sh
./microhash --test
```

### Debug build — test suite

```sh
g++ -std=c++17 -g -O0 \
    -I src/cpp \
    -o tests/cpp/microhash_tests \
    tests/cpp/microhash_tests.cpp
```

### Release build — test suite

```sh
g++ -std=c++17 -O2 -DNDEBUG \
    -I src/cpp \
    -o tests/cpp/microhash_tests \
    tests/cpp/microhash_tests.cpp
```

### Running the C++ tests

```sh
./tests/cpp/microhash_tests
```

Expected output (abbreviated):
```
=== MicroHash C++ Test Suite ===

--- Test Vectors ---
[PASS] Hello, World!
[PASS] quick brown fox
...

--- Benchmark-Style Inputs ---
[PASS] small input 'veni'
[PASS] medium input (1 KB, i%256)
[PASS] large input (1 MB, i%256)

--- Determinism ---
...

--- Sensitivity ---
...

--- Edge Cases ---
...

================================
Results: 288 passed, 0 failed
```

The process exits with code **0** on success and **1** if any test fails.

---

## C# Implementation

All `dotnet` commands can be run from the **repository root** (targeting `microhash.sln`) or from the individual project directories.

### Restore NuGet packages

```sh
dotnet restore microhash.sln
```

This only needs to be run once, or after changing `*.csproj` package references.

### Debug build

```sh
dotnet build src/csharp/microhash.csproj -c Debug
```

Or build the entire solution (main project + test project):
```sh
dotnet build microhash.sln -c Debug
```

### Release build

```sh
dotnet build src/csharp/microhash.csproj -c Release
```

### Running the CLI tool

Hash a string passed as an argument:
```sh
dotnet run --project src/csharp/microhash.csproj -- "Hello, World!"
# microhash("Hello, World!") = 0x352256EFEDC72BD1
```

Run the pre-built binary directly (after a release build):
```sh
dotnet src/csharp/bin/Release/net9.0/microhash.dll "Hello, World!"
```

Interactive mode (no arguments):
```sh
dotnet run --project src/csharp/microhash.csproj
# Enter a string to hash using MicroHash64:
```

### Debug build — test project

```sh
dotnet build tests/csharp/microhash.Tests/microhash.Tests.csproj -c Debug
```

### Running the C# tests

Run all tests for the test project directly:
```sh
dotnet test tests/csharp/microhash.Tests/microhash.Tests.csproj
```

Or run all tests in the solution at once:
```sh
dotnet test microhash.sln
```

Expected output:
```
Passed!  - Failed: 0, Passed: 32, Skipped: 0, Total: 32, Duration: ~50ms
```

The process exits with code **0** on success and non-zero if any test fails.

For more verbose output showing each test name:
```sh
dotnet test microhash.sln --logger "console;verbosity=normal"
```

### Running the benchmarks

> **Important:** BenchmarkDotNet requires a **Release** build. Running benchmarks under Debug produces a warning and unreliable numbers.

```sh
dotnet run --project src/csharp/microhash.csproj -c Release
```

Then edit `src/csharp/Program.cs` and set `bool benchmark = true;` before running, or trigger the benchmark runner directly.

---

## Docker (C# implementation)

The Dockerfile at `src/csharp/Dockerfile` performs a multi-stage build:

### Build the image

Run from the **repository root** so the build context includes the full source tree:
```sh
docker build -f src/csharp/Dockerfile -t microhash .
```

### Run the container

Hash a string:
```sh
docker run --rm microhash "Hello, World!"
```

Interactive mode:
```sh
docker run --rm -it microhash
```

---

## Test Coverage Summary

| Test group | C++ (`microhash_tests`) | C# (`HashPipeTests`) |
|---|---|---|
| Known-good test vectors (13 strings) | ✅ | ✅ |
| Benchmark inputs — small `"veni"` (4 B) | ✅ | ✅ |
| Benchmark inputs — medium 1 KB (`i%256`) | ✅ | ✅ |
| Benchmark inputs — large 1 MB (`i%256`) | ✅ | ✅ |
| Determinism — same input hashed twice | ✅ | ✅ |
| Pointer overload == vector overload | ✅ | *C++ only* |
| Sensitivity — 8 distinct-input pairs | ✅ | ✅ |
| All 256 single-byte values differ from empty | ✅ | ✅ |
| Block-boundary lengths (31, 32, 33 bytes) | ✅ | ✅ |
| **Total assertions** | **288** | **32** |

> The C++ runner counts each `ASSERT_EQ`/`ASSERT_NE` call individually (including the 256-byte loop), while xUnit counts parameterised theory cases as separate test cases.
