/**
 * @file config.h
 * @brief Configuration file parser for OpenCL framework
 *
 * Parses INI-style configuration files containing:
 * - Image processing parameters (dimensions, file paths)
 * - Algorithm selection (op_id)
 * - Kernel variant configurations (work sizes, kernel files)
 * - Buffer configurations (types, sizes)
 *
 * Configuration file format:
 * [image]
 * op_id = algorithm_name
 * input = path/to/input
 * output = path/to/output
 * src_width = 1920
 * src_height = 1080
 * kernel_x_file = path/to/kernel_x.bin
 * kernel_y_file = path/to/kernel_y.bin
 * cl_buffer_type = READ_ONLY, WRITE_ONLY
 * cl_buffer_size = 1920 * 1080 * 4, 1920 * 1080 * 4
 *
 * [kernel.v0]
 * kernel_file = path/to/kernel.cl
 * kernel_function = function_name
 * work_dim = 2
 * global_work_size = 1920,1080
 * local_work_size = 16,16
 *
 * MISRA C 2023 Compliance:
 * - Rule 21.7: Uses strtok_r (reentrant) instead of strtok
 * - Rule 21.8: Uses safe_strtol instead of atoi
 * - Rule 8.14: All structures properly tagged
 */

#pragma once

#include <stddef.h>
#include "op_interface.h"  /* For MAX_CUSTOM_BUFFERS and CustomBuffers type */

/** Maximum number of kernel configurations per algorithm */
#define MAX_KERNEL_CONFIGS 32

/* Note: HostType is defined in utils/op_interface.h */

/**
 * @brief Kernel configuration for a specific variant
 *
 * Contains all parameters needed to execute an OpenCL kernel variant,
 * including work group dimensions and kernel source location.
 */
typedef struct {
    char variant_id[32];        /**< Variant identifier (e.g., "v0", "v1") */
    char kernel_file[256];      /**< Path to OpenCL kernel source file */
    char kernel_function[64];   /**< Kernel function name to execute */
    int work_dim;               /**< Work dimensions (1, 2, or 3) */
    size_t global_work_size[3]; /**< Global work size for each dimension */
    size_t local_work_size[3];  /**< Local work group size for each dimension */
    HostType host_type;         /**< Host API type (standard or cl_extension) */
} KernelConfig;

/* Note: MAX_CUSTOM_BUFFERS is defined in utils/op_interface.h */

/** Buffer type enumeration */
typedef enum {
    BUFFER_TYPE_NONE = 0,
    BUFFER_TYPE_READ_ONLY,
    BUFFER_TYPE_WRITE_ONLY,
    BUFFER_TYPE_READ_WRITE
} BufferType;

/** Data type enumeration for buffer elements */
typedef enum {
    DATA_TYPE_NONE = 0,
    DATA_TYPE_FLOAT,    /**< 32-bit floating point (4 bytes) */
    DATA_TYPE_UCHAR,    /**< 8-bit unsigned char (1 byte) */
    DATA_TYPE_INT,      /**< 32-bit signed integer (4 bytes) */
    DATA_TYPE_SHORT     /**< 16-bit signed integer (2 bytes) */
} DataType;

/**
 * @brief Custom buffer configuration
 *
 * Describes a custom OpenCL buffer that will be created and passed to the kernel.
 * Buffers are set as kernel arguments sequentially after input/output buffers.
 * Order in config file determines kernel argument order:
 *   arg 0: input (standard)
 *   arg 1: output (standard)
 *   arg 2: first custom buffer
 *   arg 3: second custom buffer
 *   ...
 *
 * Buffers can either be:
 * - File-backed: Loaded from source_file, sized by data_type * num_elements
 * - Empty: Allocated with size_bytes, no initialization
 */
typedef struct {
    char name[64];              /**< Buffer name (from [buffer.NAME] section) */
    BufferType type;            /**< Buffer access type (READ_ONLY, WRITE_ONLY, READ_WRITE) */

    /* File-backed buffer fields */
    char source_file[256];      /**< Path to data file (empty string if not file-backed) */
    DataType data_type;         /**< Element data type (for file-backed buffers) */
    int num_elements;           /**< Number of elements (for file-backed buffers) */

    /* Empty buffer fields */
    size_t size_bytes;          /**< Direct size specification (for empty buffers) */
} CustomBufferConfig;

/**
 * @brief Scalar argument configuration
 *
 * Describes a scalar argument to be passed to the kernel (e.g., filter_width, filter_height).
 * Scalars are set as kernel arguments sequentially after all custom buffers.
 */
typedef struct {
    char name[64];              /**< Argument name */
    int value;                  /**< Scalar integer value */
} ScalarArgConfig;

/** Maximum number of scalar arguments */
#define MAX_SCALAR_ARGS 16

/**
 * @brief Complete configuration parsed from config file
 *
 * Contains all settings needed to run the OpenCL image processing
 * framework, including image parameters and all kernel variants.
 */
typedef struct {
    char op_id[32];             /**< Algorithm identifier (e.g., "dilate3x3") */
    char input_image[256];      /**< Path to input image file */
    char output_image[256];     /**< Path to output image file */
    int src_width;              /**< Source image width in pixels */
    int src_height;             /**< Source image height in pixels */
    int dst_width;              /**< Destination image width (for resize ops) */
    int dst_height;             /**< Destination image height (for resize ops) */
    int num_kernels;            /**< Number of kernel variants configured */
    KernelConfig kernels[MAX_KERNEL_CONFIGS]; /**< Array of kernel configurations */

    /* DEPRECATED: Legacy buffer files (kept for backward compatibility) */
    char kernel_x_file[256];    /**< Path to kernel_x weight file */
    char kernel_y_file[256];    /**< Path to kernel_y weight file */

    /* DEPRECATED: Legacy buffer configuration (kept for backward compatibility) */
    BufferType cl_buffer_type[MAX_CUSTOM_BUFFERS]; /**< Buffer types */
    size_t cl_buffer_size[MAX_CUSTOM_BUFFERS];     /**< Buffer sizes */
    int num_buffers;            /**< Number of configured buffers */

    /* NEW: Custom buffer configuration (replaces legacy approach) */
    CustomBufferConfig custom_buffers[MAX_CUSTOM_BUFFERS]; /**< Custom buffer configurations */
    int custom_buffer_count;    /**< Number of custom buffers configured */

    /* NEW: Scalar argument configuration */
    ScalarArgConfig scalar_args[MAX_SCALAR_ARGS]; /**< Scalar argument configurations */
    int scalar_arg_count;       /**< Number of scalar arguments configured */
} Config;

/**
 * @brief Parse configuration file
 *
 * Reads and parses an INI-style configuration file into a Config structure.
 * Supports [image] section and multiple [kernel.variant] sections.
 *
 * @param[in] filename Path to configuration file
 * @param[out] config Configuration structure to populate
 * @return 0 on success, -1 on error
 */
int parse_config(const char* filename, Config* config);

/**
 * @brief Get all kernel variants for a specific algorithm
 *
 * Extracts pointers to all kernel configurations matching the given
 * algorithm ID from the parsed configuration.
 *
 * @note MISRA C 2023: Changed signature to avoid static storage (Rule 8.7)
 *
 * @param[in] config Parsed configuration
 * @param[in] op_id Algorithm identifier to filter by
 * @param[out] variants Array of pointers to matching kernel configs
 * @param[out] count Number of variants found
 * @return 0 on success, -1 on error
 */
int get_op_variants(const Config* config, const char* op_id,
                    KernelConfig* variants[], int* count);

/**
 * @brief Resolve config path from algorithm name or explicit path
 *
 * Handles three cases:
 * 1. Algorithm name (e.g., "dilate3x3") -> "config/dilate3x3.ini"
 * 2. Explicit relative path (e.g., "config/custom.ini") -> unchanged
 * 3. Explicit absolute path (e.g., "/path/to/config.ini") -> unchanged
 *
 * Verifies that the resolved file exists.
 *
 * @param[in] input User input (algorithm name or path)
 * @param[out] output Resolved config file path
 * @param[in] output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int resolve_config_path(const char* input, char* output, size_t output_size);

/**
 * @brief Extract op_id from config file path
 *
 * Extracts the base filename without extension to use as op_id.
 * Example: "config/dilate3x3.ini" -> "dilate3x3"
 *
 * @param[in] config_path Path to config file
 * @param[out] op_id Extracted algorithm identifier
 * @param[in] op_id_size Size of op_id buffer
 * @return 0 on success, -1 on error
 */
int extract_op_id_from_path(const char* config_path, char* op_id, size_t op_id_size);
