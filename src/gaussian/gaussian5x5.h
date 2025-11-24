/**
 * @file gaussian5x5.h
 * @brief Gaussian blur with 5x5 kernel
 *
 * Implements Gaussian blur using a 5x5 convolution kernel.
 * Gaussian blur smooths images and reduces noise while preserving edges
 * better than simple averaging.
 *
 * Gaussian kernel (σ ≈ 1.0):
 * ```
 * 1  4  6  4  1
 * 4 16 24 16  4
 * 6 24 36 24  6
 * 4 16 24 16  4
 * 1  4  6  4  1
 * ```
 * Normalized by dividing by 256.
 *
 * OpenCL kernels available:
 * - gaussian0.cl: Basic implementation
 */

#pragma once

#include "../utils/op_interface.h"

/**
 * @brief Gaussian 5x5 algorithm interface
 *
 * Global algorithm object registered with the framework.
 * Provides reference implementation and verification functions.
 */
extern Algorithm gaussian5x5_algorithm;
