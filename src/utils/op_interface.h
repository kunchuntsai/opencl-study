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
 * with a consistent API, while allowing algorithm-specific parameters.
 */

#pragma once

#include <stddef.h>

/* Forward declare OpenCL types to avoid including OpenCL headers here */
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/** Border handling modes */
typedef enum {
    BORDER_CLAMP = 0,      /**< Clamp to edge (replicate edge pixels) */
    BORDER_REPLICATE = 1,  /**< Same as clamp */
    BORDER_CONSTANT = 2,   /**< Use constant border value */
    BORDER_REFLECT = 3,    /**< Reflect border pixels */
    BORDER_WRAP = 4        /**< Wrap around (periodic) */
} BorderMode;

/**
 * @brief Generic parameters for image processing operations
 *
 * Provides a flexible parameter structure that accommodates various
 * algorithm requirements while maintaining a consistent interface.
 *
 * Common use cases:
 * - Simple filters: Use input/output with src dimensions
 * - Resize operations: Different src and dst dimensions
 * - Convolution: Use algo_params for kernel weights
 * - Border-sensitive ops: Configure border_mode
 * - Verification: Use ref_output and gpu_output for comparison
 */
typedef struct {
    /* Input image */
    unsigned char* input;   /**< Input image buffer */
    int src_width;          /**< Source width in pixels */
    int src_height;         /**< Source height in pixels */
    int src_stride;         /**< Source stride in bytes (0 = packed, width * channels) */

    /* Output image */
    unsigned char* output;  /**< Output image buffer (for reference_impl) */
    int dst_width;          /**< Destination width in pixels */
    int dst_height;         /**< Destination height in pixels */
    int dst_stride;         /**< Destination stride in bytes (0 = packed, width * channels) */

    /* Verification buffers (for verify_result only) */
    unsigned char* ref_output;  /**< Reference implementation output buffer */
    unsigned char* gpu_output;  /**< GPU implementation output buffer */

    /* Border handling */
    BorderMode border_mode; /**< How to handle pixels outside image bounds */
    unsigned char border_value; /**< Constant value for BORDER_CONSTANT mode */

    /* Algorithm-specific parameters (optional extension point) */
    void* algo_params;      /**< Pointer to algorithm-specific parameters */
    size_t algo_params_size; /**< Size of algo_params structure in bytes */
} OpParams;


/**
 * @brief Kernel argument setter callback
 *
 * Sets all kernel arguments including standard buffers (input/output).
 * Called before kernel execution. All buffers are now configured via
 * .ini files rather than algorithm-specific creation callbacks.
 *
 * @param[in] kernel OpenCL kernel to set arguments for
 * @param[in] input_buf Input buffer
 * @param[in] output_buf Output buffer
 * @param[in] params Operation parameters containing dimensions
 * @return 0 on success, -1 on error
 */
typedef int (*SetKernelArgsFunc)(cl_kernel kernel,
                                 cl_mem input_buf,
                                 cl_mem output_buf,
                                 const OpParams* params);

/**
 * @brief Algorithm interface for image processing operations
 *
 * Each algorithm (dilate, gaussian, etc.) implements this interface
 * by providing function pointers for reference implementation and
 * result verification.
 *
 * The interface uses OpParams to support algorithms with varying
 * parameter requirements while maintaining a consistent API.
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
     * Algorithms should read parameters from the OpParams struct
     * and use only what they need. For example:
     * - Simple filters: Use input, output, src_width, src_height
     * - Resize: Use both src and dst dimensions
     * - Convolution: Cast algo_params to kernel-specific struct
     *
     * @param[in] params Operation parameters (input, output, dimensions, etc.)
     */
    void (*reference_impl)(const OpParams* params);

    /**
     * @brief Verify GPU result against reference
     *
     * Compares GPU output with reference implementation output
     * and calculates error metrics. Dimensions are provided
     * via OpParams to support operations with different input/output sizes.
     *
     * @param[in] params Operation parameters (contains dimensions and buffers)
     * @param[out] max_error Maximum absolute difference found
     * @return 1 if verification passed, 0 if failed
     */
    int (*verify_result)(const OpParams* params, float* max_error);

    /**
     * @brief Set kernel arguments (REQUIRED)
     *
     * Every algorithm MUST provide this callback to set its kernel arguments.
     * This ensures each algorithm explicitly declares what arguments it needs.
     *
     * The function should set all kernel arguments in order, including:
     * - Standard buffers (input, output)
     * - Additional buffers (configured via .ini files)
     * - Scalar parameters (width, height, stride, etc.)
     *
     * @see SetKernelArgsFunc
     */
    SetKernelArgsFunc set_kernel_args;
} Algorithm;
