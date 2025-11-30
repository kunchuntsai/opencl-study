/**
 * @file algorithm_runner.h
 * @brief Algorithm execution pipeline for OpenCL image processing
 *
 * Provides high-level orchestration for running image processing algorithms:
 * - C reference implementation execution
 * - Golden sample verification
 * - OpenCL kernel execution
 * - Result verification and timing
 * - Custom buffer management
 *
 * This module separates the execution logic from the CLI interface,
 * making the algorithm pipeline reusable and testable.
 */

#pragma once

#include "utils/config.h"
#include "utils/opencl_utils.h"
#include "utils/op_registry.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/** Maximum image size (used for static buffer allocation) */
#define MAX_IMAGE_SIZE (4096 * 4096)

/**
 * @brief Runtime buffer structure
 *
 * Holds OpenCL buffer handle and optional host data.
 * Used for managing custom buffers during algorithm execution.
 */
typedef struct {
    cl_mem buffer;                  /**< OpenCL buffer handle */
    unsigned char* host_data;       /**< Host data (for file-backed buffers, NULL otherwise) */
} RuntimeBuffer;

/**
 * @brief Collection of custom buffers for an algorithm
 *
 * Buffers are stored in order and set as kernel arguments sequentially:
 *   arg 0: input (standard)
 *   arg 1: output (standard)
 *   arg 2+: custom_buffers[0], custom_buffers[1], ... in order
 */
typedef struct {
    RuntimeBuffer buffers[MAX_CUSTOM_BUFFERS];  /**< Array of runtime buffers */
    int count;                                   /**< Number of buffers */
} CustomBuffers;

/**
 * @brief Run complete algorithm execution pipeline
 *
 * Executes the full algorithm pipeline:
 * 1. Loads input image
 * 2. Runs C reference implementation with timing
 * 3. Verifies against golden sample (or creates one)
 * 4. Builds and runs OpenCL kernel
 * 5. Verifies GPU results against reference
 * 6. Saves output image
 *
 * Uses static buffers for output storage (gpu_output_buffer, ref_output_buffer)
 * provided by the caller to avoid dynamic allocation (MISRA-C compliance).
 *
 * @param[in] algo Algorithm to execute
 * @param[in] kernel_cfg Kernel configuration (variant settings)
 * @param[in] config Full configuration (image paths, dimensions, buffers)
 * @param[in] env Initialized OpenCL environment
 * @param[out] gpu_output_buffer Buffer for GPU output (must be >= image size)
 * @param[out] ref_output_buffer Buffer for reference output (must be >= image size)
 */
void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                   const Config* config, OpenCLEnv* env,
                   unsigned char* gpu_output_buffer,
                   unsigned char* ref_output_buffer);
