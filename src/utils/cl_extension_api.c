/**
 * @file cl_extension_api.c
 * @brief Custom OpenCL Extension API Implementation
 *
 * This file contains dummy implementations of custom OpenCL extensions.
 * Replace these with actual custom logic as needed for specific hardware
 * or optimization requirements.
 */

#include "cl_extension_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cl_extension_init(CLExtensionContext* ctx) {
  if (ctx == NULL) {
    (void)fprintf(stderr, "Error: NULL context in cl_extension_init\n");
    return -1;
  }

  /* Initialize extension context */
  ctx->extension_data = NULL;
  ctx->initialized = 1;

  (void)printf("[CL_EXT] Custom CL extension API initialized\n");
  return 0;
}

void cl_extension_cleanup(CLExtensionContext* ctx) {
  if (ctx == NULL) {
    return;
  }

  /* Cleanup any allocated resources */
  if (ctx->extension_data != NULL) {
    /* Free extension-specific data if needed */
    ctx->extension_data = NULL;
  }

  ctx->initialized = 0;
  (void)printf("[CL_EXT] Custom CL extension API cleaned up\n");
}

cl_int cl_extension_enqueue_ndrange_kernel(
    CLExtensionContext* ctx, cl_command_queue command_queue, cl_kernel kernel,
    cl_uint work_dim, const size_t* global_work_offset,
    const size_t* global_work_size, const size_t* local_work_size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list,
    cl_event* event) {
  cl_int err;

  if (ctx == NULL) {
    (void)fprintf(stderr, "[CL_EXT] Error: NULL context\n");
    return CL_INVALID_CONTEXT;
  }

  if (!ctx->initialized) {
    (void)fprintf(stderr, "[CL_EXT] Error: Context not initialized\n");
    return CL_INVALID_CONTEXT;
  }

  /* Dummy implementation: Print information and call standard API */
  (void)printf("[CL_EXT] Custom enqueue_ndrange_kernel called\n");
  (void)printf("[CL_EXT]   Work dimensions: %u\n", work_dim);

  if (global_work_size != NULL) {
    (void)printf("[CL_EXT]   Global work size: ");
    {
      cl_uint i;
      for (i = 0; i < work_dim; i++) {
        (void)printf("%zu%s", global_work_size[i],
                     (i < work_dim - 1U) ? " x " : "\n");
      }
    }
  }

  if (local_work_size != NULL) {
    (void)printf("[CL_EXT]   Local work size: ");
    {
      cl_uint i;
      for (i = 0; i < work_dim; i++) {
        (void)printf("%zu%s", local_work_size[i],
                     (i < work_dim - 1U) ? " x " : "\n");
      }
    }
  }

  /* TODO: Add custom scheduling/optimization logic here */
  /* For now, just call the standard OpenCL API */
  err = clEnqueueNDRangeKernel(
      command_queue, kernel, work_dim, global_work_offset, global_work_size,
      local_work_size, num_events_in_wait_list, event_wait_list, event);

  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "[CL_EXT] Kernel enqueue failed: %d\n", err);
  } else {
    (void)printf("[CL_EXT] Kernel enqueued successfully\n");
  }

  return err;
}

cl_mem cl_extension_create_buffer(CLExtensionContext* ctx, cl_context context,
                                  cl_mem_flags flags, size_t size,
                                  void* host_ptr, cl_int* errcode_ret) {
  cl_mem buffer;
  const char* flag_desc = "UNKNOWN";

  if (ctx == NULL) {
    if (errcode_ret != NULL) {
      *errcode_ret = CL_INVALID_CONTEXT;
    }
    (void)fprintf(stderr, "[CL_EXT] Error: NULL context\n");
    return NULL;
  }

  if (!ctx->initialized) {
    if (errcode_ret != NULL) {
      *errcode_ret = CL_INVALID_CONTEXT;
    }
    (void)fprintf(stderr, "[CL_EXT] Error: Context not initialized\n");
    return NULL;
  }

  /* Dummy implementation: Print information and call standard API */
  (void)printf("[CL_EXT] Custom create_buffer called\n");
  (void)printf("[CL_EXT]   Buffer size: %zu bytes\n", size);

  /* Determine flag type for logging */
  if ((flags & CL_MEM_READ_ONLY) != 0U) {
    flag_desc = "READ_ONLY";
  } else if ((flags & CL_MEM_WRITE_ONLY) != 0U) {
    flag_desc = "WRITE_ONLY";
  } else if ((flags & CL_MEM_READ_WRITE) != 0U) {
    flag_desc = "READ_WRITE";
  }
  (void)printf("[CL_EXT]   Buffer flags: %s\n", flag_desc);

  /* TODO: Add custom memory allocation/pooling logic here */
  /* For now, just call the standard OpenCL API */
  buffer = clCreateBuffer(context, flags, size, host_ptr, errcode_ret);

  if ((errcode_ret != NULL) && (*errcode_ret != CL_SUCCESS)) {
    (void)fprintf(stderr, "[CL_EXT] Buffer creation failed: %d\n",
                  *errcode_ret);
  } else {
    (void)printf("[CL_EXT] Buffer created successfully\n");
  }

  return buffer;
}

cl_int cl_extension_finish(CLExtensionContext* ctx,
                           cl_command_queue command_queue) {
  cl_int err;

  if (ctx == NULL) {
    (void)fprintf(stderr, "[CL_EXT] Error: NULL context\n");
    return CL_INVALID_CONTEXT;
  }

  if (!ctx->initialized) {
    (void)fprintf(stderr, "[CL_EXT] Error: Context not initialized\n");
    return CL_INVALID_CONTEXT;
  }

  /* Dummy implementation: Print information and call standard API */
  (void)printf("[CL_EXT] Custom finish called\n");

  /* TODO: Add custom synchronization/profiling logic here */
  /* For now, just call the standard OpenCL API */
  err = clFinish(command_queue);

  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "[CL_EXT] Finish failed: %d\n", err);
  } else {
    (void)printf("[CL_EXT] Queue finished successfully\n");
  }

  return err;
}

void cl_extension_print_info(const CLExtensionContext* ctx) {
  if (ctx == NULL) {
    (void)printf("[CL_EXT] Context: NULL\n");
    return;
  }

  (void)printf("=== Custom CL Extension API Information ===\n");
  (void)printf("Initialized: %s\n", ctx->initialized ? "Yes" : "No");
  (void)printf("Extension Data: %p\n", ctx->extension_data);
  (void)printf("===========================================\n");
}
