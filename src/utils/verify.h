/**
 * @file verify.h
 * @brief Generic verification functions for image processing operations
 *
 * Provides standard verification functions that can be used by different
 * algorithms to compare GPU output against reference implementations.
 */

#pragma once

/**
 * @brief Verify exact pixel match between two images
 *
 * Checks if GPU and reference outputs match exactly (no tolerance).
 * Suitable for operations like morphological dilation that should
 * produce identical results.
 *
 * @param[in] gpu_output GPU-generated output
 * @param[in] ref_output Reference implementation output
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 * @param[out] max_error Maximum absolute difference found
 * @return 1 if verification passed, 0 if failed
 */
int verify_exact_match(unsigned char* gpu_output, unsigned char* ref_output,
                       int width, int height, float* max_error);

/**
 * @brief Verify with tolerance for floating-point operations
 *
 * Allows small differences due to floating-point rounding errors.
 * Suitable for operations like Gaussian blur that involve floating-point
 * arithmetic.
 *
 * @param[in] gpu_output GPU-generated output
 * @param[in] ref_output Reference implementation output
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 * @param[in] tolerance Maximum allowed per-pixel difference
 * @param[in] error_rate_threshold Maximum allowed error rate (0.0-1.0)
 * @param[out] max_error Maximum absolute difference found
 * @return 1 if verification passed, 0 if failed
 */
int verify_with_tolerance(unsigned char* gpu_output, unsigned char* ref_output,
                          int width, int height, float tolerance,
                          float error_rate_threshold, float* max_error);
