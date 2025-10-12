## Objective

This project is to provide a concrete example for studying openCL, and the focuses are
- How to configure kernels
- How to build the kernel code, to load the kernel binary in runtime?
- How to allocate memory that is able to share between host and kernel?
- How the host transfer data to the kernels?
- How to retrieve the kernel code result?
Please provide a simple OpenCL example to demonstrate above using a simple algorithm in OpenCV

Reference: https://github.com/opencv/opencv/wiki/OpenCL-optimizations

## Plan:
1. Create a simple but complete image thresholding example with:
    - A custom OpenCL kernel (.cl file)
    - Host code demonstrating the full OpenCL workflow
    - Detailed inline comments explaining each step
    - CMake build system with instructions for Linux/Windows
    - README explaining the learning objectives and how to use the example
2. Structure:
    - /src - Host code (C++)
    - /kernels - OpenCL kernel code (.cl files)
    - /docs - Documentation explaining OpenCL concepts
    - CMakeLists.txt - Build configuration
    - README.md - Project overview and build instructions