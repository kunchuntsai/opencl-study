/**
 * @file cl_extension_api.h
 * @brief Custom OpenCL Extension API Framework
 *
 * Provides an extensible framework for custom OpenCL host API implementations.
 * This allows switching between standard OpenCL API and custom extensions
 * based on kernel configuration.
 *
 * Configuration:
 * - Set host_type = "standard" in .ini for standard OpenCL API (default)
 * - Set host_type = "cl_extension" in .ini for custom extension API
 */

#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "config.h"

/**
 * @brief Custom CL Extension API context
 *
 * Holds any additional state needed for custom CL extension operations.
 * This is a placeholder for custom implementation-specific data.
 */
typedef struct {
  void* extension_data; /**< Custom extension-specific data */
  int initialized;      /**< Initialization flag */
} CLExtensionContext;

/**
 * @brief Initialize custom CL extension context
 *
 * Sets up any resources needed for custom CL extension operations.
 * Call this before using any custom extension APIs.
 *
 * @param[out] ctx Extension context to initialize
 * @return 0 on success, -1 on error
 */
int ClExtensionInit(CLExtensionContext* ctx);

/**
 * @brief Cleanup custom CL extension context
 *
 * Releases resources allocated for custom CL extension operations.
 *
 * @param[in,out] ctx Extension context to cleanup
 */
void ClExtensionCleanup(CLExtensionContext* ctx);

/**
 * @brief Custom implementation of clEnqueueNDRangeKernel
 *
 * This is a custom wrapper/replacement for the standard clEnqueueNDRangeKernel.
 * Implement custom scheduling, profiling, or optimization logic here.
 *
 * @param[in] ctx Custom extension context
 * @param[in] command_queue Command queue to enqueue the kernel
 * @param[in] kernel Kernel object to execute
 * @param[in] work_dim Number of work dimensions (1, 2, or 3)
 * @param[in] global_work_offset Global work offset (NULL for default)
 * @param[in] global_work_size Global work size for each dimension
 * @param[in] local_work_size Local work size for each dimension (NULL for auto)
 * @param[in] num_events_in_wait_list Number of events to wait for
 * @param[in] event_wait_list List of events to wait for
 * @param[out] event Event object for this kernel execution
 * @return CL_SUCCESS on success, error code otherwise
 */
cl_int ClExtensionEnqueueNdrangeKernel(
    CLExtensionContext* ctx, cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
    const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size,
    cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

/**
 * @brief Custom implementation of clCreateBuffer
 *
 * This is a custom wrapper/replacement for the standard clCreateBuffer.
 * Implement custom memory allocation, pooling, or optimization logic here.
 *
 * @param[in] ctx Custom extension context
 * @param[in] context OpenCL context
 * @param[in] flags Memory flags (CL_MEM_READ_ONLY, etc.)
 * @param[in] size Size of buffer in bytes
 * @param[in] host_ptr Host memory pointer (for CL_MEM_COPY_HOST_PTR, etc.)
 * @param[out] errcode_ret Error code return
 * @return Created buffer object, or NULL on error
 */
cl_mem ClExtensionCreateBuffer(CLExtensionContext* ctx, cl_context context, cl_mem_flags flags,
                                  size_t size, void* host_ptr, cl_int* errcode_ret);

/**
 * @brief Custom implementation of clFinish
 *
 * This is a custom wrapper/replacement for the standard clFinish.
 * Implement custom synchronization or profiling logic here.
 *
 * @param[in] ctx Custom extension context
 * @param[in] command_queue Command queue to finish
 * @return CL_SUCCESS on success, error code otherwise
 */
cl_int ClExtensionFinish(CLExtensionContext* ctx, cl_command_queue command_queue);

/**
 * @brief Print custom CL extension information
 *
 * Displays information about the custom CL extension API in use.
 * Useful for debugging and logging.
 *
 * @param[in] ctx Extension context
 */
void ClExtensionPrintInfo(const CLExtensionContext* ctx);
