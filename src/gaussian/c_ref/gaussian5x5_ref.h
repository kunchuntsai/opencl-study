/**
 * @file gaussian5x5_ref.h
 * @brief C reference implementation of 5x5 Gaussian blur
 *
 * Pure C implementation of Gaussian blur for:
 * - Correctness verification of GPU output
 * - Golden sample generation
 * - Performance baseline comparison
 *
 * MISRA C 2023 Compliance:
 * - Rule 9.5: Explicit array initialization
 * - Rule 18.1: Comprehensive bounds checking
 * - Rule 1.3: Integer overflow prevention
 */

#pragma once

/**
 * @brief C reference implementation of 5x5 Gaussian blur
 *
 * Applies Gaussian blur using a 5x5 convolution kernel.
 * Kernel weights approximate Gaussian distribution (σ ≈ 1.0).
 * Border pixels are handled by clamping coordinates to image bounds.
 *
 * @param[in] input Input grayscale image (width × height bytes)
 * @param[out] output Output blurred image (width × height bytes)
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 */
void gaussian5x5_ref(unsigned char* input, unsigned char* output,
                     int width, int height);
