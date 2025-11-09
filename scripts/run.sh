#!/bin/bash

# OpenCL Study Run Script
# This script runs the OpenCL operations framework
# Usage: ./run.sh [operation_number]
#   operation_number: Optional. Select operation by number (e.g., 1 for Threshold)
#                     If not provided, interactive selection will be prompted

set -e  # Exit on any error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== OpenCL Operations Run Script ===${NC}"

# Get the script directory and project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/opencl_ops"

# Get operation selection from command line argument
OPERATION_ARG=""
if [ $# -ge 1 ]; then
    OPERATION_ARG="$1"
    echo -e "${BLUE}Operation selection: $OPERATION_ARG${NC}"
fi

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
if [ -n "$OPERATION_ARG" ]; then
    echo -e "${GREEN}Running opencl_ops with operation $OPERATION_ARG...${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    ./opencl_ops "$OPERATION_ARG"
else
    echo -e "${GREEN}Running opencl_ops (interactive mode)...${NC}"
    echo -e "${BLUE}----------------------------------------${NC}"
    ./opencl_ops
fi

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
