/**
 * @file dilate3x3_ref.h
 * @brief C reference implementation of 3x3 dilation
 *
 * Pure C implementation of morphological dilation for:
 * - Correctness verification of GPU output
 * - Golden sample generation
 * - Performance baseline comparison
 *
 * MISRA C 2023 Compliance:
 * - Rule 18.1: Comprehensive bounds checking
 * - Rule 1.3: Integer overflow prevention
 */

#pragma once

/**
 * @brief C reference implementation of 3x3 dilation
 *
 * Applies morphological dilation using a 3x3 structuring element.
 * For each pixel, computes the maximum value in its 3x3 neighborhood.
 * Border pixels are handled by replicating edge values (clamp mode).
 *
 * @param[in] input Input grayscale image (width × height bytes)
 * @param[out] output Output dilated image (width × height bytes)
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 */
void dilate3x3_ref(unsigned char* input, unsigned char* output,
                   int width, int height);
