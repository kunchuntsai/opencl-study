#include "opencl_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_manager.h"
#include "cl_extension_api.h"
#include "config.h"
#include "op_interface.h"
#include "safe_ops.h"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_KERNEL_SOURCE_SIZE (1024 * 1024) /* 1MB max kernel source */
#define MAX_BUILD_LOG_SIZE (16 * 1024)       /* 16KB max build log */
#define MAX_DEVICE_NAME_SIZE 128

static char kernel_source_buffer[MAX_KERNEL_SOURCE_SIZE];
static char build_log_buffer[MAX_BUILD_LOG_SIZE];

/* Helper function to extract cache name from kernel file path */
static int extract_cache_name(const char* kernel_file, char* cache_name, size_t max_size) {
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
    /* MISRA-C:2023 Rule 17.7: Check return value */
    err = clReleaseContext(env->context);
    if (err != CL_SUCCESS) {
      (void)fprintf(stderr, "Warning: Failed to release context (error: %d)\n", err);
    }
    env->context = NULL;
    return -1;
  }

  /* Initialize custom CL extension context */
  if (cl_extension_init(&env->ext_ctx) != 0) {
    (void)fprintf(stderr, "Warning: Failed to initialize CL extension context\n");
    /* Continue anyway - standard API will still work */
  }

  (void)printf("OpenCL initialized successfully\n");
  return 0;
}

cl_kernel opencl_build_kernel(OpenCLEnv* env, const char* algorithm_id, const char* kernel_file,
                              const char* kernel_name) {
  cl_int err;
  size_t source_length;
  cl_program program = NULL;
  cl_kernel kernel;
  size_t log_size;
  const char* source_ptr;
  int used_cache = 0;
  char cache_name[256];

  if ((env == NULL) || (algorithm_id == NULL) || (kernel_file == NULL) || (kernel_name == NULL)) {
    return NULL;
  }

  /* Extract cache name from kernel file (e.g., "dilate0" from
   * "src/dilate/cl/dilate0.cl") */
  if (extract_cache_name(kernel_file, cache_name, sizeof(cache_name)) != 0) {
    (void)fprintf(stderr, "Error: Failed to extract cache name from %s\n", kernel_file);
    return NULL;
  }

  /* Check if cached kernel binary exists */
  if (cache_kernel_exists(algorithm_id, cache_name) != 0) {
    (void)printf("Found cached kernel binary for %s, loading...\n", cache_name);
    program = cache_load_kernel_binary(env->context, env->device, algorithm_id, cache_name);
    if (program != NULL) {
      used_cache = 1;
      (void)printf("Using cached kernel binary: %s\n", cache_name);
    } else {
      (void)printf("Failed to load cached binary, will compile from source\n");
    }
  }

  /* If no cached binary or loading failed, compile from source */
  if (used_cache == 0) {
    /* Read kernel source from file */
    if (read_kernel_source(kernel_file, kernel_source_buffer, MAX_KERNEL_SOURCE_SIZE,
                           &source_length) != 0) {
      return NULL;
    }

    /* Create program */
    source_ptr = kernel_source_buffer;
    program = clCreateProgramWithSource(env->context, 1U, &source_ptr, &source_length, &err);
    if (err != CL_SUCCESS) {
      (void)fprintf(stderr, "Error: Failed to create program (error code: %d)\n", err);
      return NULL;
    }

    /* Build program */
    err = clBuildProgram(program, 1U, &env->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
      (void)fprintf(stderr, "Error: Failed to build program (error code: %d)\n", err);

      /* Print build log */
      err = clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG, 0U, NULL, &log_size);
      if (err == CL_SUCCESS) {
        if (log_size <= MAX_BUILD_LOG_SIZE) {
          err = clGetProgramBuildInfo(program, env->device, CL_PROGRAM_BUILD_LOG, log_size,
                                      build_log_buffer, NULL);
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
    if (cache_save_kernel_binary(program, env->device, algorithm_id, cache_name) != 0) {
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

int opencl_run_kernel(OpenCLEnv* env, cl_kernel kernel, const Algorithm* algo, cl_mem input_buf,
                      cl_mem output_buf, const OpParams* params, const size_t* global_work_size,
                      const size_t* local_work_size, int work_dim, HostType host_type,
                      double* gpu_time_ms) {
  cl_int err;
  cl_event event;
  cl_ulong time_start;
  cl_ulong time_end;

  if ((env == NULL) || (kernel == NULL) || (params == NULL) || (gpu_time_ms == NULL)) {
    return -1;
  }

  /* Validate algorithm provides argument setter */
  if ((algo == NULL) || (algo->set_kernel_args == NULL)) {
    (void)fprintf(stderr, "Error: Algorithm must provide set_kernel_args callback\n");
    return -1;
  }

  /* Set kernel arguments using algorithm-specific setter */
  if (algo->set_kernel_args(kernel, input_buf, output_buf, params) != 0) {
    (void)fprintf(stderr, "Failed to set kernel arguments\n");
    return -1;
  }

  /* Execute kernel using appropriate API based on host_type */
  if (host_type == HOST_TYPE_CL_EXTENSION) {
    (void)printf("\n=== Using Custom CL Extension API ===\n");
    err = cl_extension_enqueue_ndrange_kernel(
        &env->ext_ctx, env->queue, kernel, (cl_uint)work_dim, NULL, global_work_size,
        (local_work_size[0] == 0U) ? NULL : local_work_size, 0U, NULL, &event);
  } else {
    /* Standard OpenCL API */
    (void)printf("\n=== Using Standard OpenCL API ===\n");
    err = clEnqueueNDRangeKernel(env->queue, kernel, (cl_uint)work_dim, NULL, global_work_size,
                                 (local_work_size[0] == 0U) ? NULL : local_work_size, 0U, NULL,
                                 &event);
  }

  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Failed to enqueue kernel (error code: %d)\n", err);
    return -1;
  }

  /* Wait for completion - use custom API if configured */
  if (host_type == HOST_TYPE_CL_EXTENSION) {
    err = cl_extension_finish(&env->ext_ctx, env->queue);
  } else {
    err = clFinish(env->queue);
  }

  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Failed to finish queue (error code: %d)\n", err);
    /* MISRA-C:2023 Rule 17.7: Check return value */
    (void)clReleaseEvent(event);
    return -1;
  }

  /* Get execution time from profiling events */
  err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start,
                                NULL);
  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Failed to get profiling start time (error code: %d)\n", err);
    /* MISRA-C:2023 Rule 17.7: Check return value */
    (void)clReleaseEvent(event);
    return -1;
  }

  err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
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

cl_mem opencl_create_buffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr,
                            const char* buffer_name) {
  cl_int err;
  cl_mem buffer;

  if ((context == NULL) || (buffer_name == NULL)) {
    (void)fprintf(stderr, "Error: Invalid parameters to opencl_create_buffer\n");
    return NULL;
  }

  buffer = clCreateBuffer(context, flags, size, host_ptr, &err);
  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Failed to create %s buffer (error code: %d)\n", buffer_name, err);
    return NULL;
  }
  return buffer;
}

void opencl_release_mem_object(cl_mem mem_obj, const char* name) {
  cl_int err;

  if (mem_obj != NULL) {
    err = clReleaseMemObject(mem_obj);
    if (err != CL_SUCCESS) {
      (void)fprintf(stderr, "Warning: Failed to release %s (error: %d)\n",
                    (name != NULL) ? name : "memory object", err);
    }
  }
}

void opencl_release_kernel(cl_kernel kernel) {
  cl_int err;

  if (kernel != NULL) {
    err = clReleaseKernel(kernel);
    if (err != CL_SUCCESS) {
      (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
    }
  }
}

void opencl_cleanup(OpenCLEnv* env) {
  cl_int err;

  if (env == NULL) {
    return;
  }

  /* Cleanup custom CL extension context */
  cl_extension_cleanup(&env->ext_ctx);

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
