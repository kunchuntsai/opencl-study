/**
 * @file opBase.h
 * @brief Header-only base class for OpenCL operations
 */

#ifndef OPBASE_H
#define OPBASE_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

/** @brief OpenCL buffer specification */
struct BufferSpec {
    void* hostPtr;       ///< Host memory pointer
    size_t size;         ///< Size in bytes
    cl_mem_flags flags;  ///< Memory flags

    BufferSpec() : hostPtr(nullptr), size(0), flags(0) {}
    BufferSpec(void* ptr, size_t sz, cl_mem_flags fl)
        : hostPtr(ptr), size(sz), flags(fl) {}
};

/** @brief Kernel argument specification (buffer or scalar) */
struct KernelArgument {
    enum Type { BUFFER, SCALAR };

    Type type;
    size_t size;
    const void* value;
    int bufferIndex;  ///< 0=input, 1=output, -1=scalar

    static KernelArgument Buffer(int index) {
        KernelArgument arg;
        arg.type = BUFFER;
        arg.bufferIndex = index;
        arg.size = 0;
        arg.value = nullptr;
        return arg;
    }

    static KernelArgument Scalar(const void* val, size_t sz) {
        KernelArgument arg;
        arg.type = SCALAR;
        arg.value = val;
        arg.size = sz;
        arg.bufferIndex = -1;
        return arg;
    }
};

/**
 * @class OpBase
 * @brief Abstract base class providing interface for OpenCL operations
 * @note Derived classes provide specifications; main.cpp handles OpenCL execution
 */
class OpBase {
public:
    OpBase() {}
    virtual ~OpBase() {}

    /** @brief Get operation name for display */
    virtual std::string getName() const = 0;

    /** @brief Get kernel source file path (default: "kernels/foo.cl") */
    virtual std::string getKernelPath() const { return "kernels/foo.cl"; }

    /** @brief Get kernel function name */
    virtual std::string getKernelName() const = 0;

    /** @brief Prepare input data (load/generate and store in member variables) */
    virtual int prepareInputData() = 0;

    /** @brief Get input buffer specification (host ptr, size, flags) */
    virtual BufferSpec getInputBufferSpec() = 0;

    /** @brief Get output buffer specification (host ptr, size, flags) */
    virtual BufferSpec getOutputBufferSpec() = 0;

    /** @brief Get kernel arguments in order (buffers and scalars) */
    virtual std::vector<KernelArgument> getKernelArguments() = 0;

    /** @brief Get NDRange work dimensions and sizes */
    virtual int getGlobalWorkSize(size_t* globalWorkSize) = 0;

    /** @brief Verify results (optional, return 0 if not needed) */
    virtual int verifyResults() = 0;

    /** @brief Load kernel source from file */
    static std::string loadKernelSource(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open kernel file: " << filename << std::endl;
            return "";
        }
        return std::string(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }
};

#endif // OPBASE_H
