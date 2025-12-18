/**
 * host.c - Simple OpenCL 1.2 host program
 *
 * This host program:
 * 1. Initializes OpenCL
 * 2. Loads and compiles the kernel with include path for utils.h
 * 3. Runs the kernel
 * 4. Displays the results
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE (1024 * 1024)
#define NUM_WORK_ITEMS 4

/* Helper function to print OpenCL errors */
const char* get_error_string(cl_int error) {
    switch(error) {
        case CL_SUCCESS: return "CL_SUCCESS";
        case CL_BUILD_PROGRAM_FAILURE: return "CL_BUILD_PROGRAM_FAILURE";
        case CL_COMPILE_PROGRAM_FAILURE: return "CL_COMPILE_PROGRAM_FAILURE";
        case CL_INVALID_PROGRAM: return "CL_INVALID_PROGRAM";
        case CL_INVALID_KERNEL_NAME: return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_VALUE: return "CL_INVALID_VALUE";
        default: return "Unknown error";
    }
}

/* Read kernel source file */
char* read_kernel_source(const char* filename, size_t* length) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open kernel file: %s\n", filename);
        return NULL;
    }

    char* source = (char*)malloc(MAX_SOURCE_SIZE);
    if (!source) {
        fclose(fp);
        return NULL;
    }

    *length = fread(source, 1, MAX_SOURCE_SIZE, fp);
    source[*length] = '\0';
    fclose(fp);

    return source;
}

int main(int argc, char** argv) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;

    int output[NUM_WORK_ITEMS] = {0};
    size_t global_work_size = NUM_WORK_ITEMS;
    size_t source_length;
    char* kernel_source;
    char build_options[512];

    printf("=== Simple OpenCL 1.2 Print Test ===\n\n");

    /* Get platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to get platform (%s)\n", get_error_string(err));
        return 1;
    }

    /* Get device (try GPU first, then CPU) */
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("No GPU found, trying CPU...\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error: Failed to get device (%s)\n", get_error_string(err));
            return 1;
        }
    }

    /* Print device info */
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Using device: %s\n\n", device_name);

    /* Create context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create context (%s)\n", get_error_string(err));
        return 1;
    }

    /* Create command queue */
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create command queue (%s)\n", get_error_string(err));
        clReleaseContext(context);
        return 1;
    }

    /* Read kernel source */
    kernel_source = read_kernel_source("kernel.cl", &source_length);
    if (!kernel_source) {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /* Create program */
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source,
                                         &source_length, &err);
    free(kernel_source);

    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create program (%s)\n", get_error_string(err));
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /*
     * Build program with include path
     * CRITICAL: The -I option tells OpenCL compiler where to find utils.h
     * We use "." (current directory) since utils.h is in the same directory as kernel.cl
     */
    snprintf(build_options, sizeof(build_options), "-I. -cl-std=CL1.2");

    printf("Build options: %s\n", build_options);
    printf("Building kernel...\n\n");

    err = clBuildProgram(program, 1, &device, build_options, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to build program (%s)\n", get_error_string(err));

        /* Get build log */
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);

        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    printf("Kernel compiled successfully!\n\n");

    /* Create kernel */
    kernel = clCreateKernel(program, "simple_print", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create kernel (%s)\n", get_error_string(err));
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /* Create output buffer */
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                    sizeof(int) * NUM_WORK_ITEMS, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create buffer (%s)\n", get_error_string(err));
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /* Set kernel argument */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to set kernel arg (%s)\n", get_error_string(err));
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /* Execute kernel */
    printf("=== Kernel Output (printf from device) ===\n\n");

    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to enqueue kernel (%s)\n", get_error_string(err));
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }

    /* Wait for completion - this also flushes printf output */
    clFinish(queue);

    printf("\n=== Host Output ===\n\n");

    /* Read back results */
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0,
                               sizeof(int) * NUM_WORK_ITEMS, output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to read buffer (%s)\n", get_error_string(err));
    } else {
        printf("Output buffer values:\n");
        for (int i = 0; i < NUM_WORK_ITEMS; i++) {
            printf("  output[%d] = %d\n", i, output[i]);
        }
    }

    printf("\n=== Test Complete ===\n");
    printf("The include of utils.h was successful!\n");

    /* Cleanup */
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
