#include "opencl_utils.h"
#include "safe_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_KERNEL_SOURCE_SIZE (1024 * 1024)  /* 1MB max kernel source */
#define MAX_BUILD_LOG_SIZE (16 * 1024)        /* 16KB max build log */
#define MAX_DEVICE_NAME_SIZE 128

static char kernel_source_buffer[MAX_KERNEL_SOURCE_SIZE];
static char build_log_buffer[MAX_BUILD_LOG_SIZE];

/* Helper function to read kernel source from file */
static int read_kernel_source(const char* filename, char* buffer, size_t max_size, size_t* length) {
    FILE* fp;
    size_t read_size;
    long file_size;

    if ((filename == NULL) || (buffer == NULL) || (length == NULL)) {
        return -1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        (void)fprintf(stderr, "Error: Failed to open kernel file: %s\n", filename);
        return -1;
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        (void)fprintf(stderr, "Error: Failed to seek in kernel file\n");
        (void)fclose(fp);
        return -1;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        (void)fprintf(stderr, "Error: Failed to get kernel file size\n");
        (void)fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        (void)fprintf(stderr, "Error: Failed to rewind kernel file\n");
        (void)fclose(fp);
        return -1;
    }

    /* Check if file size exceeds buffer */
    if ((size_t)file_size >= max_size) {
        (void)fprintf(stderr, "Error: Kernel file too large (%ld bytes, max %zu)\n",
                      file_size, max_size - 1U);
        (void)fclose(fp);
        return -1;
    }

    read_size = fread(buffer, 1U, (size_t)file_size, fp);
    buffer[read_size] = '\0';
    *length = read_size;

    if (fclose(fp) != 0) {
        (void)fprintf(stderr, "Warning: Failed to close kernel file\n");
    }

    return 0;
}

int opencl_init(OpenCLEnv* env) {
    cl_int err;
    cl_uint num_platforms;
    cl_uint num_devices;
    char device_name[MAX_DEVICE_NAME_SIZE];
    cl_command_queue_properties props;

    if (env == NULL) {
        return -1;
    }

    /* Get platform */
    err = clGetPlatformIDs(1U, &env->platform, &num_platforms);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to get platform IDs (error code: %d)\n", err);
        return -1;
    }

    /* Get device */
    err = clGetDeviceIDs(env->platform, CL_DEVICE_TYPE_GPU, 1U, &env->device, &num_devices);
    if (err != CL_SUCCESS) {
        /* Try CPU if GPU is not available */
        err = clGetDeviceIDs(env->platform, CL_DEVICE_TYPE_CPU, 1U, &env->device, &num_devices);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to get device IDs (error code: %d)\n", err);
            return -1;
        }
        (void)printf("Using CPU device\n");
    } else {
        (void)printf("Using GPU device\n");
    }

    /* Print device info */
    err = clGetDeviceInfo(env->device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    if (err == CL_SUCCESS) {
        (void)printf("Device: %s\n", device_name);
    }

    /* Create context */
    env->context = clCreateContext(NULL, 1U, &env->device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to create context (error code: %d)\n", err);
        return -1;
    }

    /* Create command queue with profiling enabled */
    props = CL_QUEUE_PROFILING_ENABLE;
    env->queue = clCreateCommandQueue(env->context, env->device, props, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to create command queue (error code: %d)\n", err);
        clReleaseContext(env->context);
        env->context = NULL;
        return -1;
    }

    (void)printf("OpenCL initialized successfully\n");
    return 0;
}

cl_kernel opencl_build_kernel(OpenCLEnv* env, const char* kernel_file,
                               const char* kernel_name) {
    cl_int err;
    size_t source_length;
    cl_program program;
    cl_kernel kernel;
    size_t log_size;
    const char* source_ptr;

    if ((env == NULL) || (kernel_file == NULL) || (kernel_name == NULL)) {
        return NULL;
    }

    /* Read kernel source from file */
    if (read_kernel_source(kernel_file, kernel_source_buffer,
                           MAX_KERNEL_SOURCE_SIZE, &source_length) != 0) {
        return NULL;
    }

    /* Create program */
    source_ptr = kernel_source_buffer;
    program = clCreateProgramWithSource(env->context, 1U,
                                       &source_ptr,
                                       &source_length, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to create program (error code: %d)\n", err);
        return NULL;
    }

    /* Build program */
    err = clBuildProgram(program, 1U, &env->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to build program (error code: %d)\n", err);

        /* Print build log */
        err = clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG,
                                    0U, NULL, &log_size);
        if (err == CL_SUCCESS) {
            if (log_size <= MAX_BUILD_LOG_SIZE) {
                err = clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG,
                                            log_size, build_log_buffer, NULL);
                if (err == CL_SUCCESS) {
                    (void)fprintf(stderr, "Build log:\n%s\n", build_log_buffer);
                }
            } else {
                (void)fprintf(stderr, "Build log too large (%zu bytes)\n", log_size);
            }
        }

        clReleaseProgram(program);
        return NULL;
    }

    /* Create kernel */
    kernel = clCreateKernel(program, kernel_name, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to create kernel '%s' (error code: %d)\n",
                      kernel_name, err);
        clReleaseProgram(program);
        return NULL;
    }

    /* We can release the program now that the kernel is created */
    clReleaseProgram(program);

    (void)printf("Built kernel '%s' from %s\n", kernel_name, kernel_file);
    return kernel;
}

int opencl_run_kernel(OpenCLEnv* env, cl_kernel kernel,
                      cl_mem input_buf, cl_mem output_buf,
                      int width, int height,
                      const size_t* global_work_size,
                      const size_t* local_work_size,
                      int work_dim,
                      double* gpu_time_ms) {
    cl_int err;
    cl_event event;
    cl_ulong time_start;
    cl_ulong time_end;

    if ((env == NULL) || (kernel == NULL) || (gpu_time_ms == NULL)) {
        return -1;
    }

    /* Set kernel arguments */
    err = clSetKernelArg(kernel, 0U, sizeof(cl_mem), &input_buf);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to set kernel arg 0 (error code: %d)\n", err);
        return -1;
    }

    err = clSetKernelArg(kernel, 1U, sizeof(cl_mem), &output_buf);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to set kernel arg 1 (error code: %d)\n", err);
        return -1;
    }

    err = clSetKernelArg(kernel, 2U, sizeof(int), &width);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to set kernel arg 2 (error code: %d)\n", err);
        return -1;
    }

    err = clSetKernelArg(kernel, 3U, sizeof(int), &height);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to set kernel arg 3 (error code: %d)\n", err);
        return -1;
    }

    /* Execute kernel */
    err = clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim,
                                 NULL, global_work_size,
                                 (local_work_size[0] == 0U) ? NULL : local_work_size,
                                 0U, NULL, &event);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to enqueue kernel (error code: %d)\n", err);
        return -1;
    }

    /* Wait for completion */
    err = clFinish(env->queue);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to finish queue (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        (void)clReleaseEvent(event);
        return -1;
    }

    /* Get execution time from profiling events */
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                   sizeof(time_start), &time_start, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to get profiling start time (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        (void)clReleaseEvent(event);
        return -1;
    }

    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                   sizeof(time_end), &time_end, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to get profiling end time (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        (void)clReleaseEvent(event);
        return -1;
    }

    /* Convert nanoseconds to milliseconds */
    *gpu_time_ms = (double)(time_end - time_start) / 1000000.0;

    /* MISRA-C:2023 Rule 17.7: Check return value */
    (void)clReleaseEvent(event);
    return 0;
}

/* MISRA-C:2023 Rule 2.2: Removed dead code (function was never called) */
/* The opencl_execute_kernel function has been removed as it was unused */

void opencl_cleanup(OpenCLEnv* env) {
    cl_int err;

    if (env == NULL) {
        return;
    }

    if (env->queue != NULL) {
        /* MISRA-C:2023 Rule 17.7: Check return value */
        err = clReleaseCommandQueue(env->queue);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release command queue (error: %d)\n", err);
        }
        env->queue = NULL;
    }

    if (env->context != NULL) {
        /* MISRA-C:2023 Rule 17.7: Check return value */
        err = clReleaseContext(env->context);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release context (error: %d)\n", err);
        }
        env->context = NULL;
    }

    (void)printf("OpenCL cleaned up\n");
}
