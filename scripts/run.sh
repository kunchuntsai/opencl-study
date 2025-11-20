#!/bin/bash

# Run script for OpenCL Image Processing Framework

cd "$(dirname "$0")/.."

if [ ! -f "build/opencl_host" ]; then
    echo "Executable not found. Please run build.sh first."
    exit 1
fi

echo "Running OpenCL Image Processing Framework..."
./build/opencl_host
