/**
 * OpenCL Image Thresholding Example
 *
 * This example demonstrates the complete OpenCL workflow:
 * 1. Platform and device selection
 * 2. Context and command queue creation
 * 3. Kernel compilation and loading
 * 4. Memory allocation (buffers)
 * 5. Data transfer (host to device)
 * 6. Kernel execution
 * 7. Result retrieval (device to host)
 * 8. Cleanup
 */

// Platform-specific OpenCL headers
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// Helper function to check OpenCL errors
#define CHECK_CL_ERROR(err, msg) \
    if (err != CL_SUCCESS) { \
        std::cerr << "OpenCL Error (" << err << "): " << msg << std::endl; \
        return -1; \
    }

// Function to load kernel source code from file
std::string loadKernelSource(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open kernel file: " << filename << std::endl;
        return "";
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

// Function to create a simple test image (grayscale gradient)
void createTestImage(std::vector<unsigned char>& image, int width, int height) {
    image.resize(width * height);

    // Create a gradient pattern for testing
    // Top-left is dark (0), bottom-right is bright (255)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            // Gradient based on position
            image[idx] = static_cast<unsigned char>(
                (x * 255 / width + y * 255 / height) / 2
            );
        }
    }
}

// Function to save image as PGM (Portable GrayMap) format
void saveImagePGM(const char* filename, const std::vector<unsigned char>& image,
                  int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create output file: " << filename << std::endl;
        return;
    }

    // PGM header
    file << "P5\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(image.data()), image.size());
    file.close();

    std::cout << "Saved image to: " << filename << std::endl;
}

int main() {
    // ========================================================================
    // STEP 1: PLATFORM AND DEVICE SELECTION
    // ========================================================================
    std::cout << "=== Step 1: Platform and Device Selection ===" << std::endl;

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;

    // Get the first available platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_ERROR(err, "Failed to get platform");

    // Get platform name for information
    char platformName[128];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, 128, platformName, NULL);
    std::cout << "Using platform: " << platformName << std::endl;

    // Get the first GPU device (or CPU if GPU not available)
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        std::cout << "No GPU found, trying CPU..." << std::endl;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    CHECK_CL_ERROR(err, "Failed to get device");

    // Get device name for information
    char deviceName[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, 128, deviceName, NULL);
    std::cout << "Using device: " << deviceName << std::endl;

    // ========================================================================
    // STEP 2: CREATE CONTEXT AND COMMAND QUEUE
    // ========================================================================
    std::cout << "\n=== Step 2: Context and Command Queue Creation ===" << std::endl;

    // Create a context - manages all OpenCL resources
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL_ERROR(err, "Failed to create context");
    std::cout << "Context created successfully" << std::endl;

    // Create a command queue - used to submit commands to the device
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL_ERROR(err, "Failed to create command queue");
    std::cout << "Command queue created successfully" << std::endl;

    // ========================================================================
    // STEP 3: LOAD AND BUILD KERNEL
    // ========================================================================
    std::cout << "\n=== Step 3: Kernel Loading and Building ===" << std::endl;

    // Load kernel source code from file
    // Note: Runtime path relative to build directory (CMake copies from src/cl/ to build/kernels/)
    std::string kernelSource = loadKernelSource("kernels/threshold.cl");
    if (kernelSource.empty()) {
        std::cerr << "Failed to load kernel source" << std::endl;
        return -1;
    }
    std::cout << "Kernel source loaded (" << kernelSource.length() << " bytes)" << std::endl;

    // Create program from source
    const char* sourcePtr = kernelSource.c_str();
    size_t sourceSize = kernelSource.length();
    cl_program program = clCreateProgramWithSource(context, 1, &sourcePtr,
                                                    &sourceSize, &err);
    CHECK_CL_ERROR(err, "Failed to create program");

    // Build (compile) the program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        // If build fails, print the build log
        char buildLog[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                             sizeof(buildLog), buildLog, NULL);
        std::cerr << "Kernel build failed:\n" << buildLog << std::endl;
        return -1;
    }
    std::cout << "Kernel compiled successfully" << std::endl;

    // Create the kernel object
    cl_kernel kernel = clCreateKernel(program, "threshold_image", &err);
    CHECK_CL_ERROR(err, "Failed to create kernel");
    std::cout << "Kernel 'threshold_image' created" << std::endl;

    // ========================================================================
    // STEP 4: PREPARE IMAGE DATA
    // ========================================================================
    std::cout << "\n=== Step 4: Image Data Preparation ===" << std::endl;

    const int WIDTH = 512;
    const int HEIGHT = 512;
    const unsigned char THRESHOLD = 128;

    // Create input test image
    std::vector<unsigned char> inputImage;
    createTestImage(inputImage, WIDTH, HEIGHT);
    std::cout << "Created test image: " << WIDTH << "x" << HEIGHT << " pixels" << std::endl;

    // Save input image for comparison
    saveImagePGM("input.pgm", inputImage, WIDTH, HEIGHT);

    // Prepare output buffer
    std::vector<unsigned char> outputImage(WIDTH * HEIGHT);

    // ========================================================================
    // STEP 5: ALLOCATE DEVICE MEMORY (BUFFERS)
    // ========================================================================
    std::cout << "\n=== Step 5: Memory Allocation on Device ===" << std::endl;

    size_t imageSize = WIDTH * HEIGHT * sizeof(unsigned char);

    // Create input buffer (read-only from kernel's perspective)
    cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                        imageSize, NULL, &err);
    CHECK_CL_ERROR(err, "Failed to create input buffer");
    std::cout << "Input buffer allocated: " << imageSize << " bytes" << std::endl;

    // Create output buffer (write-only from kernel's perspective)
    cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                         imageSize, NULL, &err);
    CHECK_CL_ERROR(err, "Failed to create output buffer");
    std::cout << "Output buffer allocated: " << imageSize << " bytes" << std::endl;

    // ========================================================================
    // STEP 6: TRANSFER DATA TO DEVICE
    // ========================================================================
    std::cout << "\n=== Step 6: Data Transfer (Host to Device) ===" << std::endl;

    // Write input image data to device buffer
    err = clEnqueueWriteBuffer(queue, inputBuffer, CL_TRUE, 0, imageSize,
                              inputImage.data(), 0, NULL, NULL);
    CHECK_CL_ERROR(err, "Failed to write input buffer");
    std::cout << "Input image transferred to device" << std::endl;

    // ========================================================================
    // STEP 7: CONFIGURE AND EXECUTE KERNEL
    // ========================================================================
    std::cout << "\n=== Step 7: Kernel Configuration and Execution ===" << std::endl;

    // Set kernel arguments
    // Argument 0: input buffer
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    CHECK_CL_ERROR(err, "Failed to set kernel arg 0");

    // Argument 1: output buffer
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    CHECK_CL_ERROR(err, "Failed to set kernel arg 1");

    // Argument 2: width
    err = clSetKernelArg(kernel, 2, sizeof(int), &WIDTH);
    CHECK_CL_ERROR(err, "Failed to set kernel arg 2");

    // Argument 3: height
    err = clSetKernelArg(kernel, 3, sizeof(int), &HEIGHT);
    CHECK_CL_ERROR(err, "Failed to set kernel arg 3");

    // Argument 4: threshold value
    err = clSetKernelArg(kernel, 4, sizeof(unsigned char), &THRESHOLD);
    CHECK_CL_ERROR(err, "Failed to set kernel arg 4");

    std::cout << "Kernel arguments configured (threshold=" << (int)THRESHOLD << ")" << std::endl;

    // Define the global work size (number of work items)
    // We use 2D work items, one for each pixel
    size_t globalWorkSize[2] = { (size_t)WIDTH, (size_t)HEIGHT };

    // Execute the kernel
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize,
                                NULL, 0, NULL, NULL);
    CHECK_CL_ERROR(err, "Failed to execute kernel");
    std::cout << "Kernel executed with " << (WIDTH * HEIGHT) << " work items" << std::endl;

    // Wait for kernel to complete
    clFinish(queue);
    std::cout << "Kernel execution completed" << std::endl;

    // ========================================================================
    // STEP 8: RETRIEVE RESULTS FROM DEVICE
    // ========================================================================
    std::cout << "\n=== Step 8: Result Retrieval (Device to Host) ===" << std::endl;

    // Read output buffer back to host memory
    err = clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, imageSize,
                             outputImage.data(), 0, NULL, NULL);
    CHECK_CL_ERROR(err, "Failed to read output buffer");
    std::cout << "Output image transferred from device" << std::endl;

    // Save output image
    saveImagePGM("output.pgm", outputImage, WIDTH, HEIGHT);

    // ========================================================================
    // STEP 9: CLEANUP
    // ========================================================================
    std::cout << "\n=== Step 9: Cleanup ===" << std::endl;

    // Release all OpenCL resources
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(outputBuffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    std::cout << "All resources released" << std::endl;
    std::cout << "\n=== SUCCESS ===" << std::endl;
    std::cout << "Check 'input.pgm' and 'output.pgm' to see the results" << std::endl;

    return 0;
}
