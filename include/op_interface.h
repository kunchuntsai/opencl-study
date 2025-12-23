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

/** Maximum number of custom buffers per algorithm */
#define MAX_CUSTOM_BUFFERS 8

/** Maximum number of custom scalars per algorithm */
#define MAX_CUSTOM_SCALARS 32

/** Buffer type enumeration */
typedef enum {
    BUFFER_TYPE_NONE = 0,
    BUFFER_TYPE_READ_ONLY,
    BUFFER_TYPE_WRITE_ONLY,
    BUFFER_TYPE_READ_WRITE
} BufferType;

/** Host type enumeration for OpenCL API selection */
typedef enum {
    HOST_TYPE_STANDARD = 0, /**< Standard OpenCL API (default) */
    HOST_TYPE_CL_EXTENSION  /**< Custom CL extension API */
} HostType;

/** Border handling modes */
typedef enum {
    BORDER_CLAMP = 0,     /**< Clamp to edge (replicate edge pixels) */
    BORDER_REPLICATE = 1, /**< Same as clamp */
    BORDER_CONSTANT = 2,  /**< Use constant border value */
    BORDER_REFLECT = 3,   /**< Reflect border pixels */
    BORDER_WRAP = 4       /**< Wrap around (periodic) */
} BorderMode;

/**
 * @brief Runtime buffer structure
 *
 * Holds OpenCL buffer handle, optional host data, and buffer configuration
 * metadata. Used for managing custom buffers during algorithm execution.
 * Configuration metadata (type, size) is needed by set_kernel_args() for
 * variant-specific argument handling (e.g., allocating local memory).
 */
typedef struct {
    char name[64];            /**< Buffer name (from config) */
    cl_mem buffer;            /**< OpenCL buffer handle */
    unsigned char* host_data; /**< Host data (for file-backed buffers, NULL otherwise) */
    BufferType type;          /**< Buffer access type (READ_ONLY, WRITE_ONLY, READ_WRITE) */
    size_t size_bytes;        /**< Buffer size in bytes */
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
    RuntimeBuffer buffers[MAX_CUSTOM_BUFFERS]; /**< Array of runtime buffers */
    int count;                                 /**< Number of buffers */
} CustomBuffers;

/** Scalar value type enumeration */
typedef enum {
    SCALAR_TYPE_NONE = 0,
    SCALAR_TYPE_INT,   /**< 32-bit signed integer */
    SCALAR_TYPE_FLOAT, /**< 32-bit floating point */
    SCALAR_TYPE_SIZE   /**< size_t (platform-dependent) */
} ScalarType;

/**
 * @brief Runtime scalar value structure
 *
 * Holds a named scalar value that can be passed to kernels.
 * Supports int, float, and size_t types.
 */
typedef struct {
    char name[64];   /**< Scalar name (from config) */
    ScalarType type; /**< Value type */
    union {
        int int_value;     /**< Integer value */
        float float_value; /**< Float value */
        size_t size_value; /**< Size value */
    } value;
} ScalarValue;

/**
 * @brief Collection of custom scalars for an algorithm
 *
 * Provides algorithm-specific scalar parameters without modifying OpParams.
 * Lookup functions check built-in OpParams fields first, then fall back
 * to custom scalars for extensibility.
 */
typedef struct {
    ScalarValue scalars[MAX_CUSTOM_SCALARS]; /**< Array of scalar values */
    int count;                               /**< Number of scalars */
} CustomScalars;

/**
 * @brief Generic parameters for image processing operations
 *
 * Provides a flexible parameter structure that accommodates various
 * algorithm requirements while maintaining a consistent interface.
 *
 * Common use cases:
 * - Simple filters: Use input/output with src dimensions
 * - Resize operations: Different src and dst dimensions
 * - Convolution: Use custom_buffers for kernel weights
 * - Border-sensitive ops: Configure border_mode
 * - Verification: Use ref_output and gpu_output for comparison
 */
typedef struct {
    /* Input image */
    unsigned char* input; /**< Input image buffer */
    int src_width;        /**< Source width in pixels */
    int src_height;       /**< Source height in pixels */
    int src_channels;     /**< Source number of channels (default 1) */
    int src_stride;       /**< Source stride in bytes (0 = packed, width * channels) */

    /* Output image */
    unsigned char* output; /**< Output image buffer (for reference_impl) */
    int dst_width;         /**< Destination width in pixels */
    int dst_height;        /**< Destination height in pixels */
    int dst_channels;      /**< Destination number of channels (default 1) */
    int dst_stride;        /**< Destination stride in bytes (0 = packed, width *
                              channels) */

    /* Verification buffers (for verify_result only) */
    unsigned char* ref_output; /**< Reference implementation output buffer */
    unsigned char* gpu_output; /**< GPU implementation output buffer */

    /* Border handling */
    BorderMode border_mode;     /**< How to handle pixels outside image bounds */
    unsigned char border_value; /**< Constant value for BORDER_CONSTANT mode */

    /* Custom buffers (for algorithms needing additional data) */
    CustomBuffers* custom_buffers; /**< Pointer to custom buffer collection (NULL if none) */

    /* Custom scalars (for algorithm-specific parameters) */
    CustomScalars* custom_scalars; /**< Pointer to custom scalar collection (NULL if none) */

    /* Kernel variant information */
    HostType host_type; /**< Host API type for current kernel variant */
    int kernel_variant; /**< Kernel signature variant ID (0, 1, 2, etc.) for
                           handling different argument layouts */
} OpParams;

/**
 * @brief Algorithm interface for image processing operations
 *
 * Each algorithm (dilate, gaussian, etc.) implements this interface
 * by providing a function pointer for the reference implementation.
 *
 * The interface uses OpParams to support algorithms with varying
 * parameter requirements while maintaining a consistent API.
 *
 * Kernel arguments are configured via .ini files using the kernel_args field.
 * Verification is configured via [verification] section in .ini files
 * with tolerance and error_rate_threshold parameters.
 */
typedef struct {
    char name[64]; /**< Human-readable name (e.g., "Dilate 3x3") */
    char id[32];   /**< Unique identifier (e.g., "dilate3x3") */

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
     * - Convolution: Access kernel weights via custom_buffers
     *
     * @param[in] params Operation parameters (input, output, dimensions, etc.)
     */
    void (*reference_impl)(const OpParams* params);
} Algorithm;
