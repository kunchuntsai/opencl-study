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

#include "op_registry.h"
#include "platform/opencl_utils.h"
#include "utils/config.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/** Maximum image size (used for static buffer allocation) */
#define MAX_IMAGE_SIZE (4096 * 4096)

/* Note: RuntimeBuffer and CustomBuffers types are defined in
 * utils/op_interface.h */

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
 * @param[out] ref_output_buffer Buffer for reference output (must be >= image
 * size)
 */
void RunAlgorithm(const Algorithm* algo, const KernelConfig* kernel_cfg, const Config* config,
                  OpenCLEnv* env, unsigned char* gpu_output_buffer,
                  unsigned char* ref_output_buffer);
