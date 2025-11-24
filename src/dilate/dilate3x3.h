/**
 * @file dilate3x3.h
 * @brief Morphological dilation with 3x3 structuring element
 *
 * Implements binary morphological dilation operation using a 3x3
 * structuring element. Dilation expands bright regions and fills
 * small holes in the image.
 *
 * Algorithm: For each pixel, take the maximum value in a 3x3 neighborhood
 *
 * OpenCL kernels available:
 * - dilate0.cl: Basic implementation
 * - dilate1.cl: Optimized with local memory
 */

#pragma once

#include "../utils/op_interface.h"

/**
 * @brief Dilate 3x3 algorithm interface
 *
 * Global algorithm object registered with the framework.
 * Provides reference implementation and verification functions.
 */
extern Algorithm dilate3x3_algorithm;
