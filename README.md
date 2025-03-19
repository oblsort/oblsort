# Oblivious Sorting
An implementation of external memory efficient, cpu instruction and memory access trace oblivious sorting algorithms.

# Prerequisites
Install cmake, ninja and intel sgx sdk, or use the cppbuilder docker image.
Requires x64 architecture (Apple M series CPU not supported).

## How to build the builder docker image
```bash
docker build -t cppbuilder:latest ./tools/docker/cppbuilder
```

## How to enter the docker environment to run algorithms in enclave
```bash
docker run -v /tmp/sortbackend:/ssdmount --privileged -it --rm -v $PWD:/builder cppbuilder
```

## How to build the unit tests in release mode

```bash
rm -rf build # optional
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

## Running benchmarks (after build) for the case data <= EPC (SGX not needed)
Run the scripts below, or use tools such as C++ TestMate in vscode.

Warning: 1TB RAM required to run the full benchmarks presented in the paper.
When benchmarking each algorithm, the input size N increases incrementally, and a C++ exception with description "std::bad_alloc" may be thrown in the test body when the RAM becomes insufficient.
Consider adjusting the range of N in perf_sort.cpp and run each benchmark individually.

### Figure 6(a) Comparing our sorting algorithm with prior works when EPC ≥ data.
```bash
./build/tests/test_basic_perf --gtest_filter=*TestSortInternalIncr*Sort*
```

### Figure 6(b) Comparing our sorting algorithm with prior works when EPC ≥ data for N = 1e8
To test the results for different element sizes, change line 14 of file osort/external_memory/algorithm/sort_def.hpp and rebuild (i.e., #define ELEMENT_SIZE 128)
```bash
./build/tests/test_basic_perf --gtest_filter=*TestSortInternal1e8*Sort*
```

### Figure 9(a) Comparing our shuffling algorithm with prior works when EPC size ≥ data size.
```bash
./build/tests/test_basic_perf --gtest_filter=*TestSortInternalIncr*Shuffle*
```

### Figure 9(b) Comparing our shuffling algorithm with prior works when EPC ≥ data for N = 1e8
To test the results for different element sizes, change line 14 of file osort/external_memory/algorithm/sort_def.hpp and rebuild (i.e., #define ELEMENT_SIZE 128)
```bash
./build/tests/test_basic_perf --gtest_filter=*TestSortInternal1e8*Shuffle*
```

### Table 3 (EPC > data): Benchmark Results for Different Applications.
```bash
./build/tests/test_apps
```

## running test/benchmark in SGX for the case data > EPC
We provide a script `algo_runner.sh` for benchmarking algorithms in SGX. Modify the script as needed to test different scenes. The script outputs result to a text file by default.
```bash
cd applications/sorting
./algo_runner.sh
# to output the terminal, run ./algo_runner.sh 1
```

## Folder structure high level details

osort - C++ osort library code

tests - C++ tests modules

applications - Enclaves example of osort

tools - tools used to generate graphs or test sets

tools/docker - dockerfiles used for reproducible builds

### OSort folder structure

common - common c++ utilies, cpu abstractions, cryptography abstractions and tracing code

external_memory - external memory abstraction and sorting algorithms

external_memory/server - server abstraction for different external memory scenarios (sgx, file system, ram)

