#!/bin/bash

# Script to build and run tests for PocketFlow-CPP

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building PocketFlow-CPP tests...${NC}"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the project
echo -e "${YELLOW}Building project...${NC}"
cmake --build . --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run tests if they were built
if [ -d "tests" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    cd tests
    
    # Run each test executable
    for test_exe in test_*; do
        if [ -x "$test_exe" ]; then
            echo -e "${YELLOW}Running $test_exe...${NC}"
            if ./"$test_exe"; then
                echo -e "${GREEN}✓ $test_exe passed${NC}"
            else
                echo -e "${RED}✗ $test_exe failed${NC}"
                exit 1
            fi
        fi
    done
    
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${YELLOW}No tests found. Make sure Google Test is installed.${NC}"
fi

echo -e "${GREEN}Build and test completed successfully!${NC}"