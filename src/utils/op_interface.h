/**
 * @file op_interface.h
 * @brief Algorithm interface definition for image processing operations
 *
 * Defines the Algorithm structure that all image processing operations
 * must implement. Each algorithm provides:
 * - C reference implementation (for correctness verification)
 * - Verification function (compares GPU vs reference output)
 * - Metadata (name, ID)
 *
 * This interface enables the framework to support multiple algorithms
 * with a consistent API.
 */

#pragma once

/**
 * @brief Algorithm interface for image processing operations
 *
 * Each algorithm (dilate, gaussian, etc.) implements this interface
 * by providing function pointers for reference implementation and
 * result verification.
 */
typedef struct {
    char name[64];              /**< Human-readable name (e.g., "Dilate 3x3") */
    char id[32];                /**< Unique identifier (e.g., "dilate3x3") */

    /**
     * @brief C reference implementation
     *
     * Executes the algorithm on CPU as a reference for correctness.
     * Used for:
     * - Generating golden samples
     * - Verifying GPU output
     * - Performance comparison
     *
     * @param[in] input Input image data (grayscale)
     * @param[out] output Output image data (grayscale)
     * @param[in] width Image width in pixels
     * @param[in] height Image height in pixels
     */
    void (*reference_impl)(unsigned char* input, unsigned char* output,
                          int width, int height);

    /**
     * @brief Verify GPU result against reference
     *
     * Compares GPU output with reference implementation output
     * and calculates error metrics.
     *
     * @param[in] gpu_output GPU-generated output
     * @param[in] ref_output Reference implementation output
     * @param[in] width Image width in pixels
     * @param[in] height Image height in pixels
     * @param[out] max_error Maximum absolute difference found
     * @return 1 if verification passed, 0 if failed
     */
    int (*verify_result)(unsigned char* gpu_output, unsigned char* ref_output,
                        int width, int height, float* max_error);

    /**
     * @brief Print algorithm information (optional)
     *
     * Displays algorithm-specific details and description.
     * Can be NULL if not needed.
     */
    void (*print_info)(void);
} Algorithm;
