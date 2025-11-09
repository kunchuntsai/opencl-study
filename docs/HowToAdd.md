## Adding New Operations

The framework makes it easy to add new operations. You only need to provide simple metadata - main.cpp handles all OpenCL flow control using your specifications. Operations automatically register themselves at startup, so no changes to `main.cpp` are needed!

### 1. Create Operation Files

```bash
# Create header file
touch src/ops/myop.h

# Create implementation file
touch src/ops/myop.cpp

# Create kernel file
touch src/cl/myop.cl
```

### 2. Implement the Operation Class

**src/ops/myop.h:**
```cpp
#ifndef MYOP_H
#define MYOP_H

#include "opBase.h"
#include <vector>

class MyOp : public OpBase {
public:
    MyOp(int width = 512, int height = 512);
    virtual ~MyOp();

    // Implement pure virtual methods - provide simple metadata
    std::string getName() const override;
    std::string getKernelPath() const override;
    std::string getKernelName() const override;
    int prepareInputData() override;
    BufferSpec getInputBufferSpec() override;
    BufferSpec getOutputBufferSpec() override;
    std::vector<KernelArgument> getKernelArguments() override;
    int getGlobalWorkSize(size_t* globalWorkSize) override;
    int verifyResults() override;

private:
    int width_, height_;
    std::vector<unsigned char> inputData_;
    std::vector<unsigned char> outputData_;
};

#endif
```

**src/ops/myop.cpp:**
```cpp
#include "myop.h"
#include "opRegistry.h"  // Include registry for auto-registration

MyOp::MyOp(int width, int height)
    : width_(width), height_(height) {
    inputData_.resize(width_ * height_);
    outputData_.resize(width_ * height_);
}

std::string MyOp::getName() const {
    return "MyOperation";
}

std::string MyOp::getKernelPath() const {
    return "kernels/myop.cl";
}

std::string MyOp::getKernelName() const {
    return "my_kernel_function";
}

int MyOp::prepareInputData() {
    // Create/load your input data here
    // ... fill inputData_ vector ...
    return 0;
}

BufferSpec MyOp::getInputBufferSpec() {
    // Return simple descriptor - main.cpp handles allocation and transfer
    return BufferSpec(inputData_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_READ_ONLY);
}

BufferSpec MyOp::getOutputBufferSpec() {
    return BufferSpec(outputData_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_WRITE_ONLY);
}

std::vector<KernelArgument> MyOp::getKernelArguments() {
    // Specify kernel arguments - main.cpp handles setting them
    std::vector<KernelArgument> args;
    args.push_back(KernelArgument::Buffer(0));  // input buffer
    args.push_back(KernelArgument::Buffer(1));  // output buffer
    args.push_back(KernelArgument::Scalar(&width_, sizeof(int)));
    args.push_back(KernelArgument::Scalar(&height_, sizeof(int)));
    return args;
}

int MyOp::getGlobalWorkSize(size_t* globalWorkSize) {
    globalWorkSize[0] = width_;
    globalWorkSize[1] = height_;
    return 2;  // 2D work dimension
}

int MyOp::verifyResults() {
    // Optional: verify results against golden reference
    return 0;
}

// ============================================================================
// AUTO-REGISTRATION
// ============================================================================
// Register this operation with the global registry at static initialization.
// This allows the operation to be automatically discovered without modifying
// main.cpp. Just add this code at the end of your operation's .cpp file!
// ============================================================================

namespace {
    // Static registrar that auto-registers MyOp at program startup
    static OpRegistrar<MyOp> registrar(
        "MyOperation",  // Name shown in menu
        []() {
            // Create MyOp with your desired parameters
            return std::unique_ptr<OpBase>(new MyOp(512, 512));
        }
    );
}
```

### 3. Build and Run

**That's it!** No changes to `main.cpp` needed. The operation automatically registers itself at startup.

```bash
./scripts/build.sh
./scripts/run.sh
```

Your new operation will automatically appear in the selection menu!

### Key Benefits of This Approach

- **Simple**: Operations are ~50 lines of metadata, not 100+ lines of OpenCL boilerplate
- **Automatic**: Self-registering operations - no manual registration in main.cpp required
- **Clean**: Focus on "what" your operation does, not "how" OpenCL works
- **Safe**: main.cpp handles all error checking and resource cleanup
- **Consistent**: All operations follow the same pattern
- **Scalable**: Add unlimited operations without modifying main.cpp's flow control code

## Code Walkthrough

### Kernel Code (src/cl/threshold.cl)

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

    if (x >= width || y >= height) return;

    // Calculate index and apply threshold
    int idx = y * width + x;
    output[idx] = (input[idx] >= threshold_value) ? 255 : 0;
}
```

### Host Code Workflow

**Steps 1-2 (main.cpp):** Platform, device, context, and queue setup

**Steps 3-8 (main.cpp):** Operation execution - main.cpp handles all OpenCL flow control
1. Load and build kernel from source (using `getKernelPath()`, `getKernelName()`)
2. Prepare input data (via operation's `prepareInputData()`)
3. Get buffer specs and allocate device memory (via `getInputBufferSpec()`, `getOutputBufferSpec()`)
4. Transfer data to device using buffer specs
5. Get kernel args and configure (via `getKernelArguments()`)
6. Execute kernel with work size (via `getGlobalWorkSize()`)
7. Retrieve results from device using buffer specs
8. Verify results (via operation's `verifyResults()`)

**Step 9 (main.cpp):** Cleanup and resource release

### Design Philosophy

The framework follows a **metadata-driven** approach with **automatic registration**:

**Separation of Concerns:**
- **main.cpp**: Handles ALL OpenCL flow control (Steps 1-8) - platform setup, buffer management, kernel execution
- **OpBase**: Defines the interface for operations to provide specifications
- **Operations**: Provide only metadata and data (inputs/outputs, kernel info, verification)
- **OpRegistry**: Automatic operation discovery via static initialization

**Automatic Registration:**
- Operations self-register at program startup using C++ static initialization
- No manual list maintenance in main.cpp required
- The registry uses a singleton pattern with factory functions
- CMake automatically compiles all `.cpp` files in `src/ops/`

**Why This Matters:**
- New operations are **simple**: ~50 lines of descriptors vs. 100+ lines of OpenCL boilerplate
- Operations focus on **business logic** (what to compute) not **infrastructure** (how OpenCL works)
- **Consistent behavior**: All error handling, logging, and resource management in one place (main.cpp)
- **Easy to maintain**: OpenCL flow changes only affect main.cpp, not every operation
- **Truly scalable**: Add operations without modifying main.cpp's flow control code