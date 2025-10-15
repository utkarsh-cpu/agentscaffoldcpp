# PocketFlow-CPP Tests

This directory contains unit tests for the PocketFlow-CPP library using Google Test framework.

## Prerequisites

- CMake 3.16 or later
- Google Test framework
- nlohmann/json library
- C++17 compatible compiler

## Building and Running Tests

### Using the test script (recommended):
```bash
./scripts/run_tests.sh
```

### Manual build:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cd tests
ctest --output-on-failure
```

### Running individual tests:
```bash
cd build/tests
./test_base_node
./test_node
./test_flow
# etc.
```

## Test Structure

- `test_base_node.cpp` - Tests for BaseNode class functionality
- `test_node.cpp` - Tests for Node class with retry logic
- `test_batch_node.cpp` - Tests for BatchNode array processing
- `test_flow.cpp` - Tests for Flow orchestration
- `test_batch_flow.cpp` - Tests for BatchFlow functionality
- `test_async_node.cpp` - Tests for AsyncNode async execution
- `test_async_batch_node.cpp` - Tests for async batch processing
- `test_async_flow.cpp` - Tests for async flow orchestration

## Installing Dependencies

### Ubuntu/Debian:
```bash
sudo apt-get install libgtest-dev nlohmann-json3-dev
```

### macOS (with Homebrew):
```bash
brew install googletest nlohmann-json
```

### Windows (with vcpkg):
```bash
vcpkg install gtest nlohmann-json
```

## CI/CD

Tests are automatically run on GitHub Actions for:
- Ubuntu (gcc, clang)
- macOS (gcc, clang) 
- Windows (MSVC)

See `.github/workflows/ci.yml` for the complete CI configuration.