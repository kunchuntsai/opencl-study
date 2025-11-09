#!/bin/bash

# OpenCL Study Run Script
# This script runs the OpenCL threshold demo

set -e  # Exit on any error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== OpenCL Study Run Script ===${NC}"

# Get the script directory and project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/threshold_demo"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Build directory not found. Running build script first...${NC}"
    "$SCRIPT_DIR/build.sh"
fi

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Executable not found. Running build script...${NC}"
    "$SCRIPT_DIR/build.sh"
fi

# Navigate to build directory (so output files are created there)
cd "$BUILD_DIR"

# Run the program
echo -e "${GREEN}Running threshold_demo...${NC}"
echo -e "${BLUE}----------------------------------------${NC}"
./threshold_demo

# Check if program ran successfully
if [ $? -eq 0 ]; then
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${GREEN}=== Program Completed Successfully ===${NC}"

    # Check for output files
    if [ -f "$BUILD_DIR/input.pgm" ] && [ -f "$BUILD_DIR/output.pgm" ]; then
        echo -e "${BLUE}Output files created:${NC}"
        echo -e "  - $BUILD_DIR/input.pgm"
        echo -e "  - $BUILD_DIR/output.pgm"
        echo -e "${YELLOW}Tip: You can view PGM files with: open input.pgm (macOS)${NC}"
    fi
else
    echo -e "${RED}=== Program Failed ===${NC}"
    exit 1
fi
