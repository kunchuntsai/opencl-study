# Simple OpenCL 1.2 Print Test

A minimal example demonstrating how to include header files in OpenCL kernels.

## Files

| File | Description |
|------|-------------|
| `host.c` | Host program that compiles and runs the kernel |
| `kernel.cl` | OpenCL kernel that includes `utils.h` |
| `include/utils.h` | Kernel header with print utility macros |

## Build

```bash
./build.sh
```

## Run

```bash
./run.sh
```

## How Kernel Include Works

The key is passing `-Iinclude` to `clBuildProgram()`:

```c
// In host.c
clBuildProgram(program, 1, &device, "-Iinclude", NULL, NULL);
```

This tells the OpenCL compiler to look in the `include/` directory for `#include` files:

```c
// In kernel.cl
#include "utils.h"
```

## Requirements

- **Ubuntu/Debian:** `sudo apt-get install opencl-headers ocl-icd-opencl-dev`
- **macOS:** OpenCL included in system
