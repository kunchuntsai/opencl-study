/**
 * @file main.cpp
 * @brief Main entry point for OpenCL operations framework
 *
 * This program demonstrates a scalable framework for OpenCL operations using
 * object-oriented design principles. The framework separates concerns:
 * - main.cpp handles platform/device/context setup (Steps 1-2)
 * - OpBase orchestrates the OpenCL workflow (Steps 3-8)
 * - Derived classes implement operation-specific behavior
 *
 * Workflow:
 * 1. Platform and device selection
 * 2. Context and command queue creation
 * 3-8. Operation execution (delegated to selected operation)
 * 9. Cleanup
 *
 * @author OpenCL Study Project
 * @date 2025
 */

// Platform-specific OpenCL headers
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <vector>
#include <memory>

// Include operation registry
#include "ops/opRegistry.h"

/**
 * @def CHECK_CL_ERROR
 * @brief Helper macro to check OpenCL API errors
 *
 * Checks if an OpenCL function returned an error code and prints
 * a descriptive message if so. Returns -1 from the current function on error.
 *
 * @param err OpenCL error code to check
 * @param msg Error message to display
 */
#define CHECK_CL_ERROR(err, msg) \
    if (err != CL_SUCCESS) { \
        std::cerr << "OpenCL Error (" << err << "): " << msg << std::endl; \
        return -1; \
    }

/**
 * @brief Main program entry point
 *
 * Sets up the OpenCL environment, presents available operations to the user,
 * and executes the selected operation.
 *
 * The program supports two modes:
 * - Interactive mode: Prompts user to select operation
 * - Command-line mode: Accepts operation number as argument
 *
 * @param argc Argument count
 * @param argv Argument vector
 *             argv[1] (optional): Operation number to execute (1-based)
 *
 * @return 0 on success, -1 on error
 *
 * Example usage:
 * @code
 * // Interactive mode
 * ./opencl_ops
 *
 * // Command-line mode (run operation 1)
 * ./opencl_ops 1
 * @endcode
 */
int main(int argc, char* argv[]) {
    // ========================================================================
    // STEP 1: PLATFORM AND DEVICE SELECTION
    // ========================================================================
    std::cout << "=== Step 1: Platform and Device Selection ===" << std::endl;

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    // Get the first available OpenCL platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_ERROR(err, "Failed to get platform");

    // Query and display platform name
    char platformName[128];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, 128, platformName, NULL);
    std::cout << "Using platform: " << platformName << std::endl;

    // Get the first GPU device, fall back to CPU if GPU not available
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        std::cout << "No GPU found, trying CPU..." << std::endl;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    CHECK_CL_ERROR(err, "Failed to get device");

    // Query and display device name
    char deviceName[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, 128, deviceName, NULL);
    std::cout << "Using device: " << deviceName << std::endl;

    // ========================================================================
    // STEP 2: CREATE CONTEXT AND COMMAND QUEUE
    // ========================================================================
    std::cout << "\n=== Step 2: Context and Command Queue Creation ===" << std::endl;

    // Create OpenCL context - manages all resources for this device
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL_ERROR(err, "Failed to create context");
    std::cout << "Context created successfully" << std::endl;

    // Create command queue - used to submit commands to the device
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL_ERROR(err, "Failed to create command queue");
    std::cout << "Command queue created successfully" << std::endl;

    // ========================================================================
    // OPERATION SETUP AND SELECTION
    // ========================================================================
    std::cout << "\n=== Available Operations ===" << std::endl;

    // Get all registered operations from the registry
    // Operations automatically register themselves at static initialization time
    // To add a new operation:
    // 1. Create a new operation class derived from OpBase
    // 2. Add auto-registration code at the end of its .cpp file (see threshold.cpp)
    // 3. Update the build system to compile the new operation
    // No changes to main.cpp are needed!
    std::vector<std::unique_ptr<OpBase>> operations = OpRegistry::instance().createAllOps();

    if (operations.empty()) {
        std::cerr << "Error: No operations registered!" << std::endl;
        std::cerr << "Make sure operation files are compiled and linked." << std::endl;
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Display available operations to user
    for (size_t i = 0; i < operations.size(); i++) {
        std::cout << i + 1 << ". " << operations[i]->getName() << std::endl;
    }

    // Parse operation selection from command line or user input
    int selection = 1;  // Default to first operation

    if (argc > 1) {
        // Command-line mode: use provided argument
        selection = std::atoi(argv[1]);
        if (selection < 1 || selection > (int)operations.size()) {
            std::cerr << "Error: Invalid operation selection: " << selection << std::endl;
            std::cerr << "Please select a number between 1 and " << operations.size() << std::endl;
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            return -1;
        }
    } else {
        // Interactive mode: prompt user for selection
        std::cout << "\nSelect operation (1-" << operations.size() << "): ";
        std::cin >> selection;

        if (selection < 1 || selection > (int)operations.size()) {
            std::cerr << "Error: Invalid selection" << std::endl;
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            return -1;
        }
    }

    // Get the selected operation
    OpBase* selectedOp = operations[selection - 1].get();
    std::cout << "\nSelected operation: " << selectedOp->getName() << std::endl;

    // ========================================================================
    // STEP 3-8: EXECUTE OPERATION
    // ========================================================================
    // main.cpp handles all OpenCL flow control:
    // - Step 3: Load and build kernel (using operation specifications)
    // - Step 4: Prepare image data (via operation)
    // - Step 5: Allocate device memory (using buffer specifications)
    // - Step 6: Transfer data to device (using buffer specifications)
    // - Step 7: Configure and execute kernel (using kernel specifications)
    // - Step 8: Retrieve results (using buffer specifications)

    std::cout << "\n=== Executing Operation: " << selectedOp->getName() << " ===" << std::endl;

    cl_program program = nullptr;
    cl_kernel kernel = nullptr;
    cl_mem inputBuffer = nullptr;
    cl_mem outputBuffer = nullptr;

    // STEP 3: LOAD AND BUILD KERNEL
    std::cout << "\n=== Step 3: Kernel Loading and Building ===" << std::endl;

    std::string kernelSource = OpBase::loadKernelSource(selectedOp->getKernelPath().c_str());
    if (kernelSource.empty()) {
        std::cerr << "Error: Failed to load kernel source from " << selectedOp->getKernelPath() << std::endl;
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Kernel source loaded (" << kernelSource.length() << " bytes)" << std::endl;

    // Create program from source
    const char* sourcePtr = kernelSource.c_str();
    size_t sourceSize = kernelSource.length();
    program = clCreateProgramWithSource(context, 1, &sourcePtr, &sourceSize, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to create program (" << err << ")" << std::endl;
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        char buildLog[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                             sizeof(buildLog), buildLog, NULL);
        std::cerr << "Error: Kernel build failed:\n" << buildLog << std::endl;
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Kernel compiled successfully" << std::endl;

    // Create kernel object
    kernel = clCreateKernel(program, selectedOp->getKernelName().c_str(), &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to create kernel '" << selectedOp->getKernelName() << "' (" << err << ")" << std::endl;
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Kernel '" << selectedOp->getKernelName() << "' created" << std::endl;

    // STEP 4: PREPARE IMAGE DATA
    std::cout << "\n=== Step 4: Image Data Preparation ===" << std::endl;
    if (selectedOp->prepareInputData() != 0) {
        std::cerr << "Error: Failed to prepare input data" << std::endl;
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Get buffer specifications from operation
    BufferSpec inputSpec = selectedOp->getInputBufferSpec();
    BufferSpec outputSpec = selectedOp->getOutputBufferSpec();

    // STEP 5: ALLOCATE DEVICE MEMORY
    std::cout << "\n=== Step 5: Memory Allocation on Device ===" << std::endl;

    inputBuffer = clCreateBuffer(context, inputSpec.flags, inputSpec.size, NULL, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to create input buffer (" << err << ")" << std::endl;
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Input buffer allocated (" << inputSpec.size << " bytes)" << std::endl;

    outputBuffer = clCreateBuffer(context, outputSpec.flags, outputSpec.size, NULL, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to create output buffer (" << err << ")" << std::endl;
        if (inputBuffer) clReleaseMemObject(inputBuffer);
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Output buffer allocated (" << outputSpec.size << " bytes)" << std::endl;

    // STEP 6: TRANSFER DATA TO DEVICE
    std::cout << "\n=== Step 6: Data Transfer (Host to Device) ===" << std::endl;

    err = clEnqueueWriteBuffer(queue, inputBuffer, CL_TRUE, 0,
                               inputSpec.size, inputSpec.hostPtr, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to write input buffer (" << err << ")" << std::endl;
        if (outputBuffer) clReleaseMemObject(outputBuffer);
        if (inputBuffer) clReleaseMemObject(inputBuffer);
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Input data transferred to device" << std::endl;

    // STEP 7: CONFIGURE AND EXECUTE KERNEL
    std::cout << "\n=== Step 7: Kernel Configuration and Execution ===" << std::endl;

    std::vector<KernelArgument> kernelArgs = selectedOp->getKernelArguments();

    for (size_t i = 0; i < kernelArgs.size(); i++) {
        const KernelArgument& arg = kernelArgs[i];

        if (arg.type == KernelArgument::BUFFER) {
            cl_mem buffer = (arg.bufferIndex == 0) ? inputBuffer : outputBuffer;
            err = clSetKernelArg(kernel, i, sizeof(cl_mem), &buffer);
        } else {
            err = clSetKernelArg(kernel, i, arg.size, arg.value);
        }

        if (err != CL_SUCCESS) {
            std::cerr << "Error: Failed to set kernel argument " << i << " (" << err << ")" << std::endl;
            if (outputBuffer) clReleaseMemObject(outputBuffer);
            if (inputBuffer) clReleaseMemObject(inputBuffer);
            if (kernel) clReleaseKernel(kernel);
            if (program) clReleaseProgram(program);
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            return -1;
        }
    }
    std::cout << "Kernel arguments configured (" << kernelArgs.size() << " arguments)" << std::endl;

    size_t globalWorkSize[3];
    int workDim = selectedOp->getGlobalWorkSize(globalWorkSize);

    err = clEnqueueNDRangeKernel(queue, kernel, workDim, NULL, globalWorkSize,
                                 NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to execute kernel (" << err << ")" << std::endl;
        if (outputBuffer) clReleaseMemObject(outputBuffer);
        if (inputBuffer) clReleaseMemObject(inputBuffer);
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Kernel executed" << std::endl;

    clFinish(queue);
    std::cout << "Kernel execution completed" << std::endl;

    // STEP 8: RETRIEVE RESULTS
    std::cout << "\n=== Step 8: Result Retrieval (Device to Host) ===" << std::endl;

    err = clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0,
                              outputSpec.size, outputSpec.hostPtr, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        std::cerr << "Error: Failed to read output buffer (" << err << ")" << std::endl;
        if (outputBuffer) clReleaseMemObject(outputBuffer);
        if (inputBuffer) clReleaseMemObject(inputBuffer);
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }
    std::cout << "Output data retrieved from device" << std::endl;

    // Verify results against golden image
    if (selectedOp->verifyResults() != 0) {
        std::cerr << "Warning: Result verification failed or not implemented" << std::endl;
    }

    // Cleanup operation-specific resources
    if (outputBuffer) clReleaseMemObject(outputBuffer);
    if (inputBuffer) clReleaseMemObject(inputBuffer);
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);

    std::cout << "\n=== Operation Completed Successfully ===" << std::endl;

    // ========================================================================
    // STEP 9: CLEANUP
    // ========================================================================
    std::cout << "\n=== Step 9: Cleanup ===" << std::endl;

    // Release OpenCL resources
    // Operation-specific resources are cleaned up by OpBase
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    std::cout << "All resources released" << std::endl;
    std::cout << "\n=== SUCCESS ===" << std::endl;

    return 0;
}
