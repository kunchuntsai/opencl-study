#include "opencl_utils.h"
#include "safe_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MISRA-C:2023 Deviation: POSIX headers required for filesystem operations
 * Rationale: ISO C does not provide APIs for directory creation or file stat.
 * These POSIX functions are necessary for binary caching functionality.
 */
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_portable(path) _mkdir(path)
#else
#include <unistd.h>
/* MISRA-C:2023 Rule 7.1: Replace octal constant with decimal */
#define mkdir_portable(path) mkdir(path, 493)  /* 493 decimal = 0755 octal = rwxr-xr-x */
#endif

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_KERNEL_SOURCE_SIZE (1024 * 1024)  /* 1MB max kernel source */
#define MAX_BUILD_LOG_SIZE (16 * 1024)        /* 16KB max build log */
#define MAX_DEVICE_NAME_SIZE 128
#define MAX_BINARY_SIZE (10 * 1024 * 1024)    /* 10MB max binary size */
#define MAX_PATH_SIZE 512
#define CACHE_DIR "cl_cache"

static char kernel_source_buffer[MAX_KERNEL_SOURCE_SIZE];
static char build_log_buffer[MAX_BUILD_LOG_SIZE];
static unsigned char binary_buffer[MAX_BINARY_SIZE];
static char cache_path_buffer[MAX_PATH_SIZE];
static char device_name_buffer[MAX_DEVICE_NAME_SIZE];

/* Get device name for cache directory */
static int get_device_name(cl_device_id device, char* name_buf, size_t buf_size) {
    cl_int err;
    size_t i;

    if ((device == NULL) || (name_buf == NULL)) {
        return -1;
    }

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, buf_size, name_buf, NULL);
    if (err != CL_SUCCESS) {
        return -1;
    }

    /* Replace spaces and special chars with underscores for filesystem compatibility */
    for (i = 0; name_buf[i] != '\0'; i++) {
        if ((name_buf[i] == ' ') || (name_buf[i] == '/') || (name_buf[i] == '\\')) {
            name_buf[i] = '_';
        }
    }

    return 0;
}

/* Get file modification time
 * Returns -1 on error, otherwise returns modification time
 * MISRA-C:2023 Deviation: Uses POSIX stat() function
 */
static time_t get_file_mtime(const char* filepath) {
    struct stat st;
    int stat_result;

    if (filepath == NULL) {
        return (time_t)-1;
    }

    stat_result = stat(filepath, &st);
    if (stat_result != 0) {
        return (time_t)-1;
    }

    return st.st_mtime;
}

/* Create cache directory structure
 * MISRA-C:2023 Deviation: Uses POSIX stat() and mkdir() functions
 */
static int create_cache_dir(const char* device_name) {
    struct stat st;
    int stat_result;
    int mkdir_result;
    int snprintf_result;

    if (device_name == NULL) {
        return -1;
    }

    /* Check if base cache directory exists */
    stat_result = stat(CACHE_DIR, &st);
    if (stat_result != 0) {
        mkdir_result = mkdir_portable(CACHE_DIR);
        if (mkdir_result != 0) {
            (void)fprintf(stderr, "Warning: Failed to create cache directory: %s\n", CACHE_DIR);
            return -1;
        }
    }

    /* Create device-specific subdirectory */
    /* MISRA-C:2023 Rule 17.7: Check snprintf return value for truncation */
    snprintf_result = snprintf(cache_path_buffer, MAX_PATH_SIZE, "%s/%s", CACHE_DIR, device_name);
    if ((snprintf_result < 0) || ((size_t)snprintf_result >= MAX_PATH_SIZE)) {
        (void)fprintf(stderr, "Warning: Cache path truncated or error\n");
        return -1;
    }

    stat_result = stat(cache_path_buffer, &st);
    if (stat_result != 0) {
        mkdir_result = mkdir_portable(cache_path_buffer);
        if (mkdir_result != 0) {
            (void)fprintf(stderr, "Warning: Failed to create device cache directory: %s\n",
                         cache_path_buffer);
            return -1;
        }
    }

    return 0;
}

/* Build cache file path from source file path
 * MISRA-C:2023 Rule 17.7: Check snprintf return value for truncation
 */
static int build_cache_path(const char* source_file, const char* device_name,
                            char* cache_file, size_t cache_file_size) {
    const char* filename;
    const char* slash;
    size_t name_len;
    int snprintf_result;

    if ((source_file == NULL) || (device_name == NULL) || (cache_file == NULL)) {
        return -1;
    }

    /* Extract filename from path */
    slash = strrchr(source_file, '/');
    if (slash != NULL) {
        filename = slash + 1;
    } else {
        filename = source_file;
    }

    /* Build cache path: CACHE_DIR/device_name/filename.bin */
    name_len = strlen(filename);
    if (name_len > 3U) {
        /* Remove .cl extension if present */
        if ((filename[name_len - 3U] == '.') &&
            (filename[name_len - 2U] == 'c') &&
            (filename[name_len - 1U] == 'l')) {
            snprintf_result = snprintf(cache_file, cache_file_size, "%s/%s/%.100s.bin",
                                      CACHE_DIR, device_name, filename);
            if ((snprintf_result < 0) || ((size_t)snprintf_result >= cache_file_size)) {
                (void)fprintf(stderr, "Warning: Cache file path truncated\n");
                return -1;
            }
            /* Manually remove the .cl before .bin */
            name_len = strlen(cache_file);
            if (name_len > 7U) {
                cache_file[name_len - 7U] = '.';
                cache_file[name_len - 6U] = 'b';
                cache_file[name_len - 5U] = 'i';
                cache_file[name_len - 4U] = 'n';
                cache_file[name_len - 3U] = '\0';
            }
        } else {
            snprintf_result = snprintf(cache_file, cache_file_size, "%s/%s/%.100s.bin",
                                      CACHE_DIR, device_name, filename);
            if ((snprintf_result < 0) || ((size_t)snprintf_result >= cache_file_size)) {
                (void)fprintf(stderr, "Warning: Cache file path truncated\n");
                return -1;
            }
        }
    } else {
        snprintf_result = snprintf(cache_file, cache_file_size, "%s/%s/%.100s.bin",
                                  CACHE_DIR, device_name, filename);
        if ((snprintf_result < 0) || ((size_t)snprintf_result >= cache_file_size)) {
            (void)fprintf(stderr, "Warning: Cache file path truncated\n");
            return -1;
        }
    }

    return 0;
}

/* Save program binary to file */
static int save_program_binary(cl_program program, cl_device_id device,
                               const char* cache_file) {
    cl_int err;
    size_t binary_size;
    unsigned char* binary_ptr;
    FILE* fp;
    size_t written;

    if ((program == NULL) || (device == NULL) || (cache_file == NULL)) {
        return -1;
    }

    /* Get binary size */
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                           sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to get binary size (error: %d)\n", err);
        return -1;
    }

    if (binary_size > MAX_BINARY_SIZE) {
        (void)fprintf(stderr, "Warning: Binary too large (%zu bytes, max %d)\n",
                     binary_size, MAX_BINARY_SIZE);
        return -1;
    }

    /* Get binary */
    binary_ptr = binary_buffer;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                           sizeof(unsigned char*), &binary_ptr, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to get binary (error: %d)\n", err);
        return -1;
    }

    /* Write binary to file */
    fp = fopen(cache_file, "wb");
    if (fp == NULL) {
        (void)fprintf(stderr, "Warning: Failed to open cache file for writing: %s\n",
                     cache_file);
        return -1;
    }

    written = fwrite(binary_buffer, 1U, binary_size, fp);
    if (written != binary_size) {
        (void)fprintf(stderr, "Warning: Failed to write complete binary (%zu/%zu bytes)\n",
                     written, binary_size);
        (void)fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        (void)fprintf(stderr, "Warning: Failed to close cache file\n");
    }

    (void)printf("Cached kernel binary to %s (%zu bytes)\n", cache_file, binary_size);
    return 0;
}

/* Load program binary from file */
static cl_program load_program_binary(cl_context context, cl_device_id device,
                                      const char* cache_file) {
    FILE* fp;
    long file_size;
    size_t read_size;
    size_t binary_size;
    const unsigned char* binary_ptr;
    cl_program program;
    cl_int err;
    cl_int binary_status;

    if ((context == NULL) || (device == NULL) || (cache_file == NULL)) {
        return NULL;
    }

    /* Open binary file */
    fp = fopen(cache_file, "rb");
    if (fp == NULL) {
        return NULL;
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        (void)fclose(fp);
        return NULL;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        (void)fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        (void)fclose(fp);
        return NULL;
    }

    if ((size_t)file_size > MAX_BINARY_SIZE) {
        (void)fprintf(stderr, "Warning: Cached binary too large (%ld bytes)\n", file_size);
        (void)fclose(fp);
        return NULL;
    }

    /* Read binary */
    binary_size = (size_t)file_size;
    read_size = fread(binary_buffer, 1U, binary_size, fp);
    if (read_size != binary_size) {
        (void)fprintf(stderr, "Warning: Failed to read complete binary\n");
        (void)fclose(fp);
        return NULL;
    }

    if (fclose(fp) != 0) {
        (void)fprintf(stderr, "Warning: Failed to close cache file\n");
    }

    /* Create program from binary */
    binary_ptr = binary_buffer;
    program = clCreateProgramWithBinary(context, 1U, &device,
                                       &binary_size, &binary_ptr,
                                       &binary_status, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to create program from binary (error: %d)\n", err);
        return NULL;
    }

    if (binary_status != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Binary status error (error: %d)\n", binary_status);
        clReleaseProgram(program);
        return NULL;
    }

    (void)printf("Loaded cached kernel binary from %s (%zu bytes)\n", cache_file, binary_size);
    return program;
}

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
    char cache_file[MAX_PATH_SIZE];
    time_t source_mtime;
    time_t cache_mtime;
    int use_cache;
    int from_cache;

    if ((env == NULL) || (kernel_file == NULL) || (kernel_name == NULL)) {
        return NULL;
    }

    program = NULL;
    use_cache = 0;
    from_cache = 0;

    /* Get device name for cache directory */
    if (get_device_name(env->device, device_name_buffer, MAX_DEVICE_NAME_SIZE) == 0) {
        /* Create cache directory */
        if (create_cache_dir(device_name_buffer) == 0) {
            /* Build cache file path */
            if (build_cache_path(kernel_file, device_name_buffer,
                                cache_file, MAX_PATH_SIZE) == 0) {
                use_cache = 1;
            }
        }
    }

    /* Try to load from cache if available */
    if (use_cache != 0) {
        source_mtime = get_file_mtime(kernel_file);
        cache_mtime = get_file_mtime(cache_file);

        /* Use cache if it exists and is newer than source
         * MISRA-C:2023: Safe time_t comparison - both must be valid (not -1)
         * and cache must be newer than or equal to source
         */
        if ((cache_mtime != (time_t)-1) && (source_mtime != (time_t)-1) &&
            (cache_mtime >= source_mtime)) {
            program = load_program_binary(env->context, env->device, cache_file);
            if (program != NULL) {
                from_cache = 1;
            }
        }
    }

    /* If cache failed or unavailable, compile from source */
    if (program == NULL) {
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
    }

    /* Build program (compiles from source or links binary) */
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

    /* Save to cache if we compiled from source */
    if ((use_cache != 0) && (from_cache == 0)) {
        (void)save_program_binary(program, env->device, cache_file);
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

    if (from_cache != 0) {
        (void)printf("Built kernel '%s' from cached binary\n", kernel_name);
    } else {
        (void)printf("Built kernel '%s' from source: %s\n", kernel_name, kernel_file);
    }

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
