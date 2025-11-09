#!/bin/bash

# OpenCL Study Build Script
# This script builds the OpenCL project using CMake

set -e  # Exit on any error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== OpenCL Study Build Script ===${NC}"

# Get the script directory and project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo -e "${BLUE}Project root: $PROJECT_ROOT${NC}"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Run CMake configuration
echo -e "${GREEN}Running CMake configuration...${NC}"
cmake ..

# Build the project
echo -e "${GREEN}Building project...${NC}"
cmake --build .

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}=== Build Successful ===${NC}"
    echo -e "${BLUE}Executable location: $BUILD_DIR/threshold_demo${NC}"
    echo -e "${BLUE}To run: cd build && ./threshold_demo${NC}"
else
    echo -e "${RED}=== Build Failed ===${NC}"
    exit 1
fi
