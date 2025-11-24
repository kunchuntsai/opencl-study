#!/bin/bash

# Run script for OpenCL Image Processing Framework

cd "$(dirname "$0")/.."

if [ ! -f "build/opencl_host" ]; then
    echo "Executable not found. Please run build.sh first."
    exit 1
fi

# If argument provided, use it as variant index
if [ $# -ge 1 ]; then
    echo "Running with variant $1 (algorithm from config.ini)..."
    ./build/opencl_host "$1"
    exit $?
fi

# No arguments - run interactively (program will prompt for variant)
echo "Running OpenCL Image Processing Framework (algorithm from config.ini)..."
echo ""
./build/opencl_host
