#!/bin/bash

# Run script for OpenCL Image Processing Framework

cd "$(dirname "$0")/.."

if [ ! -f "build/opencl_host" ]; then
    echo "Executable not found. Please run build.sh first."
    exit 1
fi

# If no arguments provided, run with defaults
if [ $# -eq 0 ]; then
    echo "Running with default parameters (algorithm=0, variant=0)..."
    ./build/opencl_host
    exit $?
fi

# If arguments are provided, use them directly
if [ $# -ge 2 ]; then
    echo "Running OpenCL Image Processing Framework..."
    ./build/opencl_host "$1" "$2"
    exit $?
fi

# Interactive mode - prompt user for input
echo "=== OpenCL Image Processing Framework ==="
echo ""
echo "Available algorithms:"
echo "  0 - dilate3x3"
echo "  1 - gaussian5x5"
echo ""
read -p "Select algorithm index (default: 0): " algo_index
algo_index=${algo_index:-0}

echo ""
echo "Available variants:"
echo "  0 - v0 (basic)"
echo "  1 - v1 (optimized, if available)"
echo ""
read -p "Select variant index (default: 0): " variant_index
variant_index=${variant_index:-0}

echo ""
echo "Running algorithm $algo_index with variant $variant_index..."
./build/opencl_host "$algo_index" "$variant_index"
