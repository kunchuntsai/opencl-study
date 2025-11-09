/**
 * @file opBase.h
 * @brief Header-only base class for OpenCL image operations
 *
 * This file provides the OpBase abstract base class that implements the
 * template method pattern for OpenCL operations. The base class handles
 * the complete OpenCL workflow orchestration (steps 3-8) while derived
 * classes implement operation-specific behavior through pure virtual methods.
 *
 * @author OpenCL Study Project
 * @date 2025
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

/**
 * @struct BufferSpec
 * @brief Specification for an OpenCL buffer
 *
 * Describes a buffer's host memory location, size, and OpenCL memory flags.
 * Used by operations to specify their input/output buffer requirements.
 */
struct BufferSpec {
    void* hostPtr;       ///< Pointer to host memory
    size_t size;         ///< Buffer size in bytes
    cl_mem_flags flags;  ///< OpenCL memory flags (CL_MEM_READ_ONLY, etc.)

    BufferSpec() : hostPtr(nullptr), size(0), flags(0) {}
    BufferSpec(void* ptr, size_t sz, cl_mem_flags fl)
        : hostPtr(ptr), size(sz), flags(fl) {}
};

/**
 * @struct KernelArgument
 * @brief Specification for a kernel argument
 *
 * Describes a single kernel argument which can be either a buffer or scalar value.
 */
struct KernelArgument {
    enum Type { BUFFER, SCALAR };

    Type type;           ///< Type of argument (buffer or scalar)
    size_t size;         ///< Size of scalar argument (0 for buffers)
    const void* value;   ///< Pointer to scalar value (nullptr for buffers)
    int bufferIndex;     ///< Buffer index (0=input, 1=output, -1=not a buffer)

    // Constructor for buffer arguments
    static KernelArgument Buffer(int index) {
        KernelArgument arg;
        arg.type = BUFFER;
        arg.bufferIndex = index;
        arg.size = 0;
        arg.value = nullptr;
        return arg;
    }

    // Constructor for scalar arguments
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
 * @brief Abstract base class for OpenCL image operations
 *
 * Provides an interface for implementing OpenCL operations with clean
 * separation of concerns. Derived classes only need to provide:
 * - Input/output buffer specifications
 * - Kernel metadata (path, name, arguments)
 * - Data preparation and verification
 *
 * All OpenCL flow control (buffer allocation, data transfer, kernel execution)
 * is handled by main.cpp, which queries the operation for its specifications.
 *
 * This design keeps operation classes clean and focused on their specific
 * logic without any OpenCL flow control code.
 *
 * @note This is a header-only class for simplicity
 * @note Operations provide specifications; main.cpp handles execution
 *
 * Example usage:
 * @code
 * class MyOp : public OpBase {
 *     std::string getName() const override { return "MyOperation"; }
 *     BufferSpec getInputBufferSpec() override { return BufferSpec(...); }
 *     // Implement other simple metadata methods...
 * };
 *
 * // In main.cpp:
 * MyOp* op = ...;
 * op->prepareInputData();
 * // Use op->getInputBufferSpec(), op->getKernelPath(), etc.
 * // main.cpp handles all OpenCL operations
 * @endcode
 */
class OpBase {
public:
    /** @brief Default constructor */
    OpBase() {}

    /** @brief Virtual destructor for proper cleanup of derived classes */
    virtual ~OpBase() {}

    // ========================================================================
    // Pure Virtual Methods - Must be implemented by derived classes
    // ========================================================================
    // Operations provide simple metadata and data descriptors.
    // main.cpp uses these specifications to handle all OpenCL flow control.
    // ========================================================================

    /**
     * @brief Get the operation name for display purposes
     * @return Human-readable operation name (e.g., "Threshold", "Blur")
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the path to the OpenCL kernel source file
     *
     * Default implementation returns "kernels/foo.cl". Override this method
     * to specify a custom kernel file path.
     *
     * @return Path to kernel file relative to executable
     * @note Path is relative to the build directory
     */
    virtual std::string getKernelPath() const { return "kernels/foo.cl"; }

    /**
     * @brief Get the kernel function name to execute
     * @return Name of the kernel function in the .cl file
     * @note Must match the __kernel function name exactly
     */
    virtual std::string getKernelName() const = 0;

    /**
     * @brief Prepare input data for the operation
     *
     * This method should:
     * - Load or generate input data
     * - Store data in member variables for later use
     * - Optionally save input data to files for debugging
     *
     * @return 0 on success, -1 on error
     * @note Called before buffer allocation
     */
    virtual int prepareInputData() = 0;

    /**
     * @brief Get input buffer specification
     *
     * Returns a descriptor specifying:
     * - Host memory pointer to input data
     * - Buffer size in bytes
     * - OpenCL memory flags (typically CL_MEM_READ_ONLY)
     *
     * OpBase uses this spec to allocate device buffer and transfer data.
     *
     * @return BufferSpec describing input buffer requirements
     *
     * @note Called after prepareInputData(), so data should be ready
     * @note hostPtr must point to valid memory of size 'size'
     *
     * Example:
     * @code
     * BufferSpec getInputBufferSpec() override {
     *     return BufferSpec(inputImage_.data(),
     *                      width_ * height_,
     *                      CL_MEM_READ_ONLY);
     * }
     * @endcode
     */
    virtual BufferSpec getInputBufferSpec() = 0;

    /**
     * @brief Get output buffer specification
     *
     * Returns a descriptor specifying:
     * - Host memory pointer to receive output data
     * - Buffer size in bytes
     * - OpenCL memory flags (typically CL_MEM_WRITE_ONLY)
     *
     * OpBase uses this spec to allocate device buffer and retrieve results.
     *
     * @return BufferSpec describing output buffer requirements
     *
     * @note hostPtr must point to valid memory of size 'size'
     * @note Memory will be filled when results are retrieved from device
     *
     * Example:
     * @code
     * BufferSpec getOutputBufferSpec() override {
     *     return BufferSpec(outputImage_.data(),
     *                      width_ * height_,
     *                      CL_MEM_WRITE_ONLY);
     * }
     * @endcode
     */
    virtual BufferSpec getOutputBufferSpec() = 0;

    /**
     * @brief Get kernel arguments specification
     *
     * Returns a vector describing all kernel arguments in order.
     * Each argument can be either a buffer reference or a scalar value.
     *
     * OpBase uses this to configure kernel arguments before execution.
     *
     * @return Vector of KernelArgument descriptors
     *
     * @note Arguments must be in same order as kernel function signature
     * @note Use KernelArgument::Buffer(index) for buffers (0=input, 1=output)
     * @note Use KernelArgument::Scalar(ptr, size) for scalar values
     *
     * Example:
     * @code
     * // For kernel: threshold_image(global uchar* in, global uchar* out,
     * //                            int width, int height, uchar threshold)
     * std::vector<KernelArgument> getKernelArguments() override {
     *     std::vector<KernelArgument> args;
     *     args.push_back(KernelArgument::Buffer(0));  // input buffer
     *     args.push_back(KernelArgument::Buffer(1));  // output buffer
     *     args.push_back(KernelArgument::Scalar(&width_, sizeof(int)));
     *     args.push_back(KernelArgument::Scalar(&height_, sizeof(int)));
     *     args.push_back(KernelArgument::Scalar(&threshold_, sizeof(unsigned char)));
     *     return args;
     * }
     * @endcode
     */
    virtual std::vector<KernelArgument> getKernelArguments() = 0;

    /**
     * @brief Get global work size and work dimension for kernel execution
     *
     * Define the NDRange (number of work items) for kernel execution.
     *
     * @param[out] globalWorkSize Array to receive work size for each dimension
     *                            (must have space for at least 3 elements)
     *
     * @return Work dimension (1, 2, or 3)
     *
     * @note For 2D images: return 2 and set globalWorkSize[0] = width, [1] = height
     * @note Total work items = globalWorkSize[0] * globalWorkSize[1] * ...
     *
     * Example:
     * @code
     * int getGlobalWorkSize(size_t* globalWorkSize) override {
     *     globalWorkSize[0] = width_;
     *     globalWorkSize[1] = height_;
     *     return 2;  // 2D work dimension
     * }
     * @endcode
     */
    virtual int getGlobalWorkSize(size_t* globalWorkSize) = 0;

    /**
     * @brief Verify results against golden reference (optional)
     *
     * Compare operation output against expected golden image or values.
     * This is useful for testing and validation.
     *
     * @return 0 if verification passes or not implemented, -1 on verification failure
     * @note Implementation is optional - can return 0 if not needed
     * @note Called after results have been retrieved to host memory
     */
    virtual int verifyResults() = 0;

    // ========================================================================
    // Static Helper Utilities
    // ========================================================================
    // These are provided as convenience functions for derived classes
    // and for main.cpp to use.
    // ========================================================================

    /**
     * @brief Load kernel source code from file
     *
     * Static helper utility to read the entire contents of a kernel source file
     * into a string for compilation. Can be used by main.cpp or derived classes.
     *
     * @param[in] filename Path to kernel file
     * @return Kernel source code as string, or empty string on error
     *
     * @note Prints error message to stderr if file cannot be opened
     */
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
