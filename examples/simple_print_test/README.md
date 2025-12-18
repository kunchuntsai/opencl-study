# Simple OpenCL 1.2 Print Test

A minimal example demonstrating how to include header files in OpenCL kernels.

## Files

| File | Description |
|------|-------------|
| `host.c` | Host program that compiles and runs the kernel |
| `kernel.cl` | OpenCL kernel that includes `utils.h` |
| `utils.h` | Kernel header with print utility functions |
| `CL/cl.h` | Minimal OpenCL header stub (for offline builds) |

## Build

```bash
make
```

## Run

```bash
./simple_print_test
```

## How Kernel Include Works

The key is passing `-I.` to `clBuildProgram()`:

```c
// In host.c
clBuildProgram(program, 1, &device, "-I. -cl-std=CL1.2", NULL, NULL);
```

This tells the OpenCL compiler to look in the current directory for `#include` files, allowing:

```c
// In kernel.cl
#include "utils.h"
```

## Requirements

For full OpenCL runtime:
- **Ubuntu/Debian:** `sudo apt-get install opencl-headers ocl-icd-opencl-dev`
- **macOS:** OpenCL included in system
