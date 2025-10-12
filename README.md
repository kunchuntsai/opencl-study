# OpenCL Study Project

A comprehensive example for studying OpenCL development workflow using a simple image thresholding algorithm.

## Learning Objectives

This project demonstrates the complete OpenCL development workflow:

1. **Kernel Configuration** - How to write and structure OpenCL kernels
2. **Building Kernels** - How to compile and load kernel code at runtime
3. **Memory Allocation** - How to allocate device memory that can be shared between host and kernel
4. **Data Transfer** - How the host transfers data to kernels
5. **Result Retrieval** - How to retrieve kernel execution results

## Project Structure

```
.
├── CMakeLists.txt       # Build configuration
├── README.md            # This file
├── src/
│   └── main.cpp         # Host code demonstrating OpenCL workflow
├── kernels/
│   └── threshold.cl     # OpenCL kernel for image thresholding
└── docs/
    ├── opencl_concepts.md  # Detailed OpenCL concepts explanation
    └── STUDY_PLAN.md       # Step-by-step learning guide
```

## Algorithm: Image Thresholding

The example implements a simple image thresholding algorithm:
- Input: Grayscale image with pixel values 0-255
- Operation: For each pixel, if value >= threshold, set to 255 (white), else set to 0 (black)
- Output: Binary (black and white) image

This simple algorithm allows you to focus on understanding the OpenCL workflow rather than complex image processing logic.

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install build-essential cmake

# Install OpenCL headers and ICD loader
sudo apt install opencl-headers ocl-icd-opencl-dev

# Install OpenCL driver (choose based on your hardware)

# For Intel CPUs/GPUs:
sudo apt install intel-opencl-icd

# For NVIDIA GPUs:
sudo apt install nvidia-opencl-dev

# For AMD GPUs:
sudo apt install mesa-opencl-icd
```

### Windows

1. **Install Visual Studio** (2017 or later) with C++ development tools

2. **Install CMake**
   - Download from https://cmake.org/download/
   - Add to PATH during installation

3. **Install OpenCL SDK**
   - Intel: Download Intel SDK for OpenCL Applications
   - NVIDIA: Install CUDA Toolkit (includes OpenCL)
   - AMD: Download AMD APP SDK

4. **Set Environment Variables** (if needed):
   ```
   OPENCL_ROOT=C:\path\to\opencl\sdk
   ```

### Verify OpenCL Installation

Check available OpenCL platforms:

```bash
# Linux
sudo apt install clinfo
clinfo

# Windows (after installing SDK)
# Use vendor-specific tools or download clinfo
```

## Building the Project

### Linux/macOS

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make

# Run the example
./threshold_demo
```

### Windows (Visual Studio)

```batch
# Create build directory
mkdir build
cd build

# Generate Visual Studio solution
cmake -G "Visual Studio 16 2019" ..

# Build with MSBuild
cmake --build . --config Release

# Run the example
Release\threshold_demo.exe
```

## Expected Output

When you run the program, you'll see output like:

```
=== Step 1: Platform and Device Selection ===
Using platform: Intel(R) OpenCL
Using device: Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz

=== Step 2: Context and Command Queue Creation ===
Context created successfully
Command queue created successfully

=== Step 3: Kernel Loading and Building ===
Kernel source loaded (1234 bytes)
Kernel compiled successfully
Kernel 'threshold_image' created

=== Step 4: Image Data Preparation ===
Created test image: 512x512 pixels
Saved image to: input.pgm

=== Step 5: Memory Allocation on Device ===
Input buffer allocated: 262144 bytes
Output buffer allocated: 262144 bytes

=== Step 6: Data Transfer (Host to Device) ===
Input image transferred to device

=== Step 7: Kernel Configuration and Execution ===
Kernel arguments configured (threshold=128)
Kernel executed with 262144 work items
Kernel execution completed

=== Step 8: Result Retrieval (Device to Host) ===
Output image transferred from device
Saved image to: output.pgm

=== Step 9: Cleanup ===
All resources released

=== SUCCESS ===
Check 'input.pgm' and 'output.pgm' to see the results
```

The program will generate two PGM image files:
- `input.pgm` - Original grayscale gradient image
- `output.pgm` - Thresholded binary image

You can view these files with image viewers like GIMP, ImageMagick, or online PGM viewers.

## Code Walkthrough

### Kernel Code (kernels/threshold.cl)

The kernel is executed in parallel for each pixel:

```c
__kernel void threshold_image(
    __global const uchar* input,
    __global uchar* output,
    const int width,
    const int height,
    const uchar threshold_value)
{
    // Get pixel coordinates
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Calculate index and apply threshold
    int idx = y * width + x;
    output[idx] = (input[idx] >= threshold_value) ? 255 : 0;
}
```

### Host Code Structure (src/main.cpp)

The host code follows a 9-step workflow:

1. **Platform/Device Selection** - Choose OpenCL platform and device (GPU/CPU)
2. **Context Creation** - Create execution context for managing resources
3. **Command Queue** - Create queue for submitting commands
4. **Kernel Loading** - Load and compile kernel from .cl file
5. **Memory Allocation** - Allocate buffers on device
6. **Data Transfer (H→D)** - Copy input data to device
7. **Kernel Execution** - Set arguments and execute kernel
8. **Data Transfer (D→H)** - Copy results back to host
9. **Cleanup** - Release all resources

Each step is clearly commented in the source code.

## Troubleshooting

### "Failed to get platform" error
- OpenCL runtime is not installed
- Install appropriate OpenCL driver for your hardware

### "Failed to create program" or "Kernel build failed"
- Check kernel syntax in `threshold.cl`
- Build log will show compilation errors

### "No GPU found, trying CPU..." message
- This is normal if no GPU is available
- The program will fall back to CPU execution

### CMake can't find OpenCL
```bash
# Linux: Specify OpenCL path manually
cmake -DOpenCL_LIBRARY=/path/to/libOpenCL.so -DOpenCL_INCLUDE_DIR=/path/to/CL ..

# Windows: Set environment variable
set CMAKE_PREFIX_PATH=C:\path\to\opencl\sdk
```

## Further Learning

For more detailed explanations of OpenCL concepts used in this example, see:
- [docs/STUDY_PLAN.md](docs/STUDY_PLAN.md) - Step-by-step learning guide with 8 progressive phases
- [docs/opencl_concepts.md](docs/opencl_concepts.md) - In-depth concept explanations
- [OpenCV OpenCL Optimizations](https://github.com/opencv/opencv/wiki/OpenCL-optimizations) - Reference documentation

## Note for macOS Users

Apple deprecated OpenCL in favor of Metal. While this code demonstrates OpenCL concepts, it may not build/run on modern macOS systems (especially M-series chips). This is intended as a study resource for understanding OpenCL concepts that can be built on Linux/Windows systems.

## License

This is a study project for educational purposes.
