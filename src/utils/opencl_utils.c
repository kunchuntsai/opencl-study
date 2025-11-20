#include "opencl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to read kernel source from file
static char* read_kernel_source(const char* filename, size_t* length) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open kernel file: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* source = (char*)malloc(*length + 1);
    if (!source) {
        fprintf(stderr, "Error: Failed to allocate memory for kernel source\n");
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(source, 1, *length, fp);
    source[read_size] = '\0';
    *length = read_size;

    fclose(fp);
    return source;
}

int opencl_init(OpenCLEnv* env) {
    cl_int err;
    cl_uint num_platforms, num_devices;

    // Get platform
    err = clGetPlatformIDs(1, &env->platform, &num_platforms);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to get platform IDs (error code: %d)\n", err);
        return -1;
    }

    // Get device
    err = clGetDeviceIDs(env->platform, CL_DEVICE_TYPE_GPU, 1, &env->device, &num_devices);
    if (err != CL_SUCCESS) {
        // Try CPU if GPU is not available
        err = clGetDeviceIDs(env->platform, CL_DEVICE_TYPE_CPU, 1, &env->device, &num_devices);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error: Failed to get device IDs (error code: %d)\n", err);
            return -1;
        }
        printf("Using CPU device\n");
    } else {
        printf("Using GPU device\n");
    }

    // Print device info
    char device_name[128];
    clGetDeviceInfo(env->device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Device: %s\n", device_name);

    // Create context
    env->context = clCreateContext(NULL, 1, &env->device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create context (error code: %d)\n", err);
        return -1;
    }

    // Create command queue with profiling enabled
    cl_command_queue_properties props = CL_QUEUE_PROFILING_ENABLE;
    env->queue = clCreateCommandQueue(env->context, env->device, props, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create command queue (error code: %d)\n", err);
        clReleaseContext(env->context);
        return -1;
    }

    printf("OpenCL initialized successfully\n");
    return 0;
}

cl_kernel opencl_load_kernel(OpenCLEnv* env, const char* kernel_file,
                            const char* kernel_name) {
    cl_int err;
    size_t source_length;

    // Read kernel source from file
    char* source = read_kernel_source(kernel_file, &source_length);
    if (!source) {
        return NULL;
    }

    // Create program
    cl_program program = clCreateProgramWithSource(env->context, 1,
                                                   (const char**)&source,
                                                   &source_length, &err);
    free(source);

    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create program (error code: %d)\n", err);
        return NULL;
    }

    // Build program
    err = clBuildProgram(program, 1, &env->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to build program (error code: %d)\n", err);

        // Print build log
        size_t log_size;
        clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG,
                            0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG,
                            log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);

        clReleaseProgram(program);
        return NULL;
    }

    // Create kernel
    cl_kernel kernel = clCreateKernel(program, kernel_name, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create kernel '%s' (error code: %d)\n",
                kernel_name, err);
        clReleaseProgram(program);
        return NULL;
    }

    // We can release the program now that the kernel is created
    clReleaseProgram(program);

    printf("Loaded kernel '%s' from %s\n", kernel_name, kernel_file);
    return kernel;
}

int opencl_execute_kernel(OpenCLEnv* env, cl_kernel kernel,
                         cl_mem input_buf, cl_mem output_buf,
                         int work_dim, size_t* global_size, size_t* local_size) {
    cl_int err;

    err = clEnqueueNDRangeKernel(env->queue, kernel, work_dim, NULL,
                                global_size, local_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to enqueue kernel (error code: %d)\n", err);
        return -1;
    }

    clFinish(env->queue);
    return 0;
}

void opencl_cleanup(OpenCLEnv* env) {
    if (env->queue) clReleaseCommandQueue(env->queue);
    if (env->context) clReleaseContext(env->context);
    printf("OpenCL cleaned up\n");
}
