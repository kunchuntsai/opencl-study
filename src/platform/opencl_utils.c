/**
 * @file opencl_utils.c
 * @brief OpenCL utility functions for device initialization and kernel execution
 *
 * Provides high-level wrappers around OpenCL API for:
 * - Platform and device initialization
 * - Kernel compilation and caching
 * - Kernel execution with timing
 * - Resource cleanup
 *
 * Kernel argument handling is in kernel_args.c for better organization.
 *
 * MISRA C 2023 Compliance:
 * - Rule 17.7: All OpenCL API return values checked
 * - Rule 21.3: Uses static buffers for kernel source
 * - Rule 22.x: Proper resource management and cleanup
 */

#include "opencl_utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_manager.h"
#include "kernel_args.h"

/* Fallback for non-CMake builds - assumes running from project root */
#ifndef CL_INCLUDE_DIR
#define CL_INCLUDE_DIR "include/cl"
#endif
#include "cl_extension_api.h"
#include "op_interface.h"
#include "utils/config.h"
#include "utils/safe_ops.h"

/* ============================================================================
 * Static Buffer Definitions
 * ============================================================================
 * MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation
 */

#define MAX_KERNEL_SOURCE_SIZE (1024 * 1024) /* 1MB max kernel source */
#define MAX_BUILD_LOG_SIZE (16 * 1024)       /* 16KB max build log */
#define MAX_DEVICE_NAME_SIZE 128
#define MAX_HEADER_SOURCE_SIZE (256 * 1024) /* 256KB max for embedded headers */

static char kernel_source_buffer[MAX_KERNEL_SOURCE_SIZE];
static char combined_source_buffer[MAX_KERNEL_SOURCE_SIZE + MAX_HEADER_SOURCE_SIZE];
static char build_log_buffer[MAX_BUILD_LOG_SIZE];

/* ============================================================================
 * Kernel Source File Handling
 * ============================================================================ */

/**
 * @brief Extract cache name from kernel file path
 *
 * Extracts the filename without extension for use as cache identifier.
 *
 * @param kernel_file Full path to kernel file
 * @param cache_name Output buffer for cache name
 * @param max_size Maximum buffer size
 * @return 0 on success, -1 on error
 */
static int ExtractCacheName(const char* kernel_file, char* cache_name, size_t max_size) {
    const char* filename_start;
    const char* ext_start;
    size_t name_len;

    if ((kernel_file == NULL) || (cache_name == NULL) || (max_size == 0U)) {
        return -1;
    }

    /* Find the last '/' to get filename */
    filename_start = strrchr(kernel_file, '/');
    if (filename_start != NULL) {
        filename_start++; /* Skip the '/' */
    } else {
        filename_start = kernel_file; /* No path separator found */
    }

    /* Find the extension */
    ext_start = strrchr(filename_start, '.');
    if (ext_start != NULL) {
        name_len = (size_t)(ext_start - filename_start);
    } else {
        name_len = strlen(filename_start);
    }

    /* Check if name fits in buffer */
    if (name_len >= max_size) {
        (void)fprintf(stderr, "Error: Cache name too long\n");
        return -1;
    }

    /* Copy the name without extension */
    (void)strncpy(cache_name, filename_start, name_len);
    cache_name[name_len] = '\0';

    return 0;
}

/**
 * @brief Read kernel source from file
 *
 * Reads the contents of a kernel source file into a buffer.
 *
 * @param filename Path to the kernel file
 * @param buffer Output buffer for source code
 * @param max_size Maximum buffer size
 * @param length Output parameter for actual length read
 * @return 0 on success, -1 on error
 */
static int ReadKernelSource(const char* filename, char* buffer, size_t max_size, size_t* length) {
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
        (void)fprintf(stderr, "Error: Kernel file too large (%ld bytes, max %zu)\n", file_size,
                      max_size - 1U);
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

#define MAX_INCLUDES 16
#define MAX_HEADER_NAME 64

/**
 * @brief Read kernel source with headers embedded based on #include directives
 *
 * Scans the kernel source for #include "xxx.h" directives, embeds matching
 * headers from include/cl/, and comments out the #include lines.
 *
 * Kernel developers use standard #include syntax:
 *   #include "platform.h"
 *   #include "types.h"
 *
 * This function will:
 * 1. Read the kernel source
 * 2. Find all #include "xxx.h" directives
 * 3. Prepend the header contents from include/cl/
 * 4. Comment out the #include lines in the kernel
 *
 * @param[in] kernel_file Path to the main kernel source file
 * @param[out] buffer Output buffer for combined source
 * @param[in] max_size Maximum buffer size
 * @param[out] length Total length of combined source
 * @param[in] host_type Host API type (for future extension)
 * @return 0 on success, -1 on error
 */
static int EmbedHeaders(const char* kernel_file, char* buffer, size_t max_size, size_t* length,
                        HostType host_type) {
    size_t total_length = 0;
    size_t kernel_length;
    size_t header_length;
    char header_names[MAX_INCLUDES][MAX_HEADER_NAME];
    int header_count = 0;
    char* src;
    char* line_start;
    char header_path[512];
    int i;

    if ((kernel_file == NULL) || (buffer == NULL) || (length == NULL)) {
        return -1;
    }

    (void)(host_type); /* Reserved for future use */

    buffer[0] = '\0';

    /* Step 1: Read kernel source into kernel_source_buffer */
    if (ReadKernelSource(kernel_file, kernel_source_buffer, MAX_KERNEL_SOURCE_SIZE,
                         &kernel_length) != 0) {
        return -1;
    }

    /* Step 2: Scan for #include "xxx.h", check if exists in include/cl/, collect and comment out */
    src = kernel_source_buffer;
    while ((line_start = strstr(src, "#include")) != NULL) {
        char* quote_start;
        char* quote_end;
        char* line_begin;
        size_t name_len;
        char temp_name[MAX_HEADER_NAME];
        FILE* test_fp;

        /* Find the beginning of this line to check for comments */
        line_begin = line_start;
        while ((line_begin > kernel_source_buffer) && (*(line_begin - 1) != '\n')) {
            line_begin--;
        }

        /* Check if line is commented out (// before #include) */
        {
            char* comment_check = line_begin;
            int is_commented = 0;
            while (comment_check < line_start) {
                if ((comment_check[0] == '/') && (comment_check[1] == '/')) {
                    is_commented = 1;
                    break;
                }
                comment_check++;
            }
            if (is_commented != 0) {
                src = line_start + 8; /* skip past "#include" */
                continue;
            }
        }

        quote_start = strchr(line_start, '"');
        if (quote_start == NULL) {
            src = line_start + 8;
            continue;
        }
        quote_end = strchr(quote_start + 1, '"');
        if (quote_end == NULL) {
            src = line_start + 8;
            continue;
        }

        name_len = (size_t)(quote_end - quote_start - 1);
        if ((name_len == 0) || (name_len >= MAX_HEADER_NAME)) {
            src = quote_end + 1;
            continue;
        }

        /* Extract header name */
        (void)strncpy(temp_name, quote_start + 1, name_len);
        temp_name[name_len] = '\0';

        /* Check if header exists in include/cl/ */
        (void)snprintf(header_path, sizeof(header_path), "%s/%s", CL_INCLUDE_DIR, temp_name);
        test_fp = fopen(header_path, "r");
        if (test_fp != NULL) {
            /* Header exists - add to list and comment out the #include line */
            (void)fclose(test_fp);

            if (header_count < MAX_INCLUDES) {
                (void)strncpy(header_names[header_count], temp_name, MAX_HEADER_NAME - 1);
                header_names[header_count][MAX_HEADER_NAME - 1] = '\0';
                header_count++;
            }

            /* Comment out the #include line */
            *line_start = '/';
            *(line_start + 1) = '/';
        }
        /* If header doesn't exist in include/cl/, leave #include as-is for runtime */

        src = quote_end + 1;
    }

    /* Step 3: Prepend each header from include/cl/ */
    for (i = 0; i < header_count; i++) {
        (void)snprintf(header_path, sizeof(header_path), "%s/%s", CL_INCLUDE_DIR, header_names[i]);

        if (ReadKernelSource(header_path, buffer + total_length, max_size - total_length,
                             &header_length) != 0) {
            (void)fprintf(stderr, "Error: Failed to read header: %s\n", header_path);
            return -1;
        }

        total_length += header_length;
        buffer[total_length++] = '\n';
        buffer[total_length] = '\0';
    }

    /* Step 4: Append the kernel source (with embedded #includes commented out) */
    if ((total_length + kernel_length + 1U) >= max_size) {
        (void)fprintf(stderr, "Error: Combined source exceeds buffer size\n");
        return -1;
    }

    (void)memcpy(buffer + total_length, kernel_source_buffer, kernel_length);
    total_length += kernel_length;
    buffer[total_length] = '\0';

    *length = total_length;
    return 0;
}

/* ============================================================================
 * OpenCL Environment Lifecycle
 * ============================================================================ */

int OpenclInit(OpenCLEnv* env) {
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
        /* MISRA-C:2023 Rule 17.7: Check return value */
        err = clReleaseContext(env->context);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release context (error: %d)\n", err);
        }
        env->context = NULL;
        return -1;
    }

    /* Initialize custom CL extension context */
    if (ClExtensionInit(&env->ext_ctx) != 0) {
        (void)fprintf(stderr, "Warning: Failed to initialize CL extension context\n");
        /* Continue anyway - standard API will still work */
    }

    (void)printf("OpenCL initialized successfully\n");
    return 0;
}

/* ============================================================================
 * Kernel Building
 * ============================================================================ */

cl_kernel OpenclBuildKernel(OpenCLEnv* env, const char* algorithm_id,
                            const struct KernelConfig* kernel_cfg) {
    cl_int err;
    size_t source_length;
    cl_program program = NULL;
    cl_kernel kernel;
    size_t log_size;
    const char* source_ptr;
    int used_cache = 0;
    char cache_name[256];
    char build_options[512];
    const char* kernel_file;
    const char* kernel_name;
    const char* kernel_option;
    HostType host_type;

    if ((env == NULL) || (algorithm_id == NULL) || (kernel_cfg == NULL)) {
        return NULL;
    }

    /* Extract parameters from KernelConfig */
    kernel_file = kernel_cfg->kernel_file;
    kernel_name = kernel_cfg->kernel_function;
    kernel_option = kernel_cfg->kernel_option;
    host_type = kernel_cfg->host_type;

    /* Extract cache name from kernel file (e.g., "dilate0" from
     * "src/dilate/cl/dilate0.cl") */
    if (ExtractCacheName(kernel_file, cache_name, sizeof(cache_name)) != 0) {
        (void)fprintf(stderr, "Error: Failed to extract cache name from %s\n", kernel_file);
        return NULL;
    }

    /* Check if cached kernel binary exists AND source hasn't changed */
    if (CacheKernelIsValid(algorithm_id, cache_name, kernel_file) != 0) {
        (void)printf("Found valid cached kernel binary for %s, loading...\n", cache_name);
        program = CacheLoadKernelBinary(env->context, env->device, algorithm_id, cache_name);
        if (program != NULL) {
            used_cache = 1;
            (void)printf("Using cached kernel binary: %s\n", cache_name);
        } else {
            (void)printf("Failed to load cached binary, will compile from source\n");
        }
    }

    /* Construct build options: "<user_options> -DHOST_TYPE=N" */
    {
        const char* user_opts = (kernel_option != NULL) ? kernel_option : "";
        int host_type_val = (host_type == HOST_TYPE_CL_EXTENSION) ? 1 : 0;

        (void)snprintf(build_options, sizeof(build_options), "%s -DHOST_TYPE=%d", user_opts,
                       host_type_val);
        (void)printf("Kernel build options: %s\n", build_options);
    }

    /* If no cached binary or loading failed, compile from source */
    if (used_cache == 0) {
        /* Read kernel source with embedded platform headers */
        if (EmbedHeaders(kernel_file, combined_source_buffer, sizeof(combined_source_buffer),
                         &source_length, host_type) != 0) {
            return NULL;
        }
        source_ptr = combined_source_buffer;

        /* Create program */
        program = clCreateProgramWithSource(env->context, 1U, &source_ptr, &source_length, &err);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to create program (error code: %d)\n", err);
            return NULL;
        }

        /* Build program with options */
        err = clBuildProgram(program, 1U, &env->device, build_options, NULL, NULL);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to build program (error code: %d)\n", err);

            /* Print build log */
            err = clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG, 0U, NULL,
                                        &log_size);
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

            /* MISRA-C:2023 Rule 17.7: Check return value */
            err = clReleaseProgram(program);
            if (err != CL_SUCCESS) {
                (void)fprintf(stderr, "Warning: Failed to release program (error: %d)\n", err);
            }
            return NULL;
        }

        (void)printf("Kernel compiled successfully\n");

        /* Save compiled binary to cache for future runs */
        if (CacheSaveKernelBinary(program, env->device, algorithm_id, cache_name) == 0) {
            /* Also save source hash for change detection */
            unsigned char source_hash[CACHE_HASH_SIZE];
            if (CacheComputeSourceHash(kernel_file, source_hash) == 0) {
                if (CacheSaveSourceHash(algorithm_id, cache_name, source_hash) != 0) {
                    (void)fprintf(stderr, "Warning: Failed to save source hash\n");
                }
            } else {
                (void)fprintf(stderr, "Warning: Failed to compute source hash\n");
            }
        } else {
            (void)fprintf(stderr, "Warning: Failed to cache kernel binary\n");
        }
    }

    /* Create kernel */
    kernel = clCreateKernel(program, kernel_name, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to create kernel '%s' (error code: %d)\n", kernel_name,
                      err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        err = clReleaseProgram(program);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release program (error: %d)\n", err);
        }
        return NULL;
    }

    /* We can release the program now that the kernel is created */
    /* MISRA-C:2023 Rule 17.7: Check return value */
    err = clReleaseProgram(program);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to release program (error: %d)\n", err);
    }

    if (used_cache == 0) {
        (void)printf("Built kernel '%s' from %s (cached as %s)\n", kernel_name, kernel_file,
                     cache_name);
    }
    return kernel;
}

/* ============================================================================
 * Kernel Execution
 * ============================================================================ */

int OpenclRunKernel(OpenCLEnv* env, cl_kernel kernel, const Algorithm* algo, cl_mem input_buf,
                    cl_mem output_buf, const OpParams* params,
                    const struct KernelConfig* kernel_cfg, double* gpu_time_ms) {
    cl_int err;
    cl_event event;
    cl_ulong time_start;
    cl_ulong time_end;
    const size_t* global_work_size;
    const size_t* local_work_size;
    int work_dim;
    HostType host_type;

    if ((env == NULL) || (kernel == NULL) || (params == NULL) || (gpu_time_ms == NULL)) {
        return -1;
    }

    /* Set kernel arguments using kernel_cfg */
    if (kernel_cfg == NULL) {
        (void)fprintf(stderr, "Error: kernel_cfg is required for setting kernel arguments\n");
        return -1;
    }

    /* Extract parameters from KernelConfig */
    global_work_size = kernel_cfg->global_work_size;
    local_work_size = kernel_cfg->local_work_size;
    work_dim = kernel_cfg->work_dim;
    host_type = kernel_cfg->host_type;

    if (OpenclSetKernelArgs(kernel, input_buf, output_buf, params, kernel_cfg) != 0) {
        (void)fprintf(stderr, "Failed to set kernel arguments from config\n");
        return -1;
    }

    /* Execute kernel using appropriate API based on host_type */
    if (host_type == HOST_TYPE_STANDARD) {
        (void)printf("\n=== Using Standard OpenCL API ===\n");
        err = clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim, NULL, global_work_size,
                                     (local_work_size[0] == 0U) ? NULL : local_work_size, 0U, NULL,
                                     &event);
    } else {
        (void)printf("\n=== Using Custom CL Extension API ===\n");
        err = ClExtensionEnqueueNdrangeKernel(
            &env->ext_ctx, env->queue, kernel, (cl_uint)work_dim, NULL, global_work_size,
            (local_work_size[0] == 0U) ? NULL : local_work_size, 0U, NULL, &event);

            CacheSaveCustomBinary(&env->ext_ctx);
    }

    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to enqueue kernel (error code: %d)\n", err);
        return -1;
    }

    /* Wait for completion - use custom API if configured */
    err = clFinish(env->queue);

    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to finish queue (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        (void)clReleaseEvent(event);
        return -1;
    }

    /* Get execution time from profiling events */
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start),
                                  &time_start, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to get profiling start time (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        (void)clReleaseEvent(event);
        return -1;
    }

    err =
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
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

/* ============================================================================
 * Buffer Management
 * ============================================================================ */

cl_mem OpenclCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr,
                          const char* buffer_name) {
    cl_int err;
    cl_mem buffer;

    if ((context == NULL) || (buffer_name == NULL)) {
        (void)fprintf(stderr, "Error: Invalid parameters to OpenclCreateBuffer\n");
        return NULL;
    }

    buffer = clCreateBuffer(context, flags, size, host_ptr, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to create %s buffer (error code: %d)\n", buffer_name, err);
        return NULL;
    }
    return buffer;
}

void OpenclReleaseMemObject(cl_mem mem_obj, const char* name) {
    cl_int err;

    if (mem_obj != NULL) {
        err = clReleaseMemObject(mem_obj);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release %s (error: %d)\n",
                          (name != NULL) ? name : "memory object", err);
        }
    }
}

void OpenclReleaseKernel(cl_kernel kernel) {
    cl_int err;

    if (kernel != NULL) {
        err = clReleaseKernel(kernel);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
        }
    }
}

/* ============================================================================
 * Cleanup
 * ============================================================================ */

void OpenclCleanup(OpenCLEnv* env) {
    cl_int err;

    if (env == NULL) {
        return;
    }

    /* Cleanup custom CL extension context */
    ClExtensionCleanup(&env->ext_ctx);

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
