#!/bin/bash

# Build script for OpenCL Image Processing Framework

cd "$(dirname "$0")/../src"

echo "Building OpenCL Image Processing Framework..."
make clean
make

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable: build/opencl_host"
else
    echo "Build failed!"
    exit 1
fi
