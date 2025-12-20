/**
 * @file kernel_args.h
 * @brief Kernel argument handling for OpenCL kernels
 *
 * Provides functions to set kernel arguments based on configuration.
 * Separates argument mapping logic from core OpenCL operations.
 *
 * MISRA C 2023 Compliance:
 * - Rule 17.7: All OpenCL API return values checked
 */

#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "op_interface.h"
#include "utils/config.h"

/**
 * @brief Set kernel arguments for OpenCL kernel
 *
 * Sets kernel arguments for an OpenCL kernel using configuration from kernel_config.
 * If kernel_config has kernel_args configured, uses that; otherwise falls back to
 * default behavior (input, output, width, height).
 *
 * Supports multiple argument types:
 * - BUFFER_INPUT: Primary input buffer
 * - BUFFER_OUTPUT: Primary output buffer
 * - BUFFER_CUSTOM: Named custom buffers from OpParams
 * - SCALAR_INT: Integer scalars from OpParams fields or custom_scalars
 * - SCALAR_FLOAT: Float scalars from custom_scalars
 * - SCALAR_SIZE: Size_t scalars (buffer sizes or custom values)
 * - STRUCT: Packed struct from multiple scalar fields
 *
 * @param[in] kernel OpenCL kernel to set arguments for
 * @param[in] input_buf Input buffer containing image data
 * @param[in] output_buf Output buffer for processed image
 * @param[in] params Operation parameters (dimensions, algo-specific data)
 * @param[in] kernel_config Kernel configuration with argument descriptors
 * @return 0 on success, -1 on error
 */
int OpenclSetKernelArgs(cl_kernel kernel, cl_mem input_buf, cl_mem output_buf,
                        const OpParams* params, const KernelConfig* kernel_config);
