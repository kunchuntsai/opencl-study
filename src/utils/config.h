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

/** Maximum number of kernel configurations per algorithm */
#define MAX_KERNEL_CONFIGS 32

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
} KernelConfig;

/** Maximum number of custom buffers */
#define MAX_CUSTOM_BUFFERS 8

/** Buffer type enumeration */
typedef enum {
    BUFFER_TYPE_NONE = 0,
    BUFFER_TYPE_READ_ONLY,
    BUFFER_TYPE_WRITE_ONLY,
    BUFFER_TYPE_READ_WRITE
} BufferType;

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

    /* Custom buffer files (e.g., kernel weights) */
    char kernel_x_file[256];    /**< Path to kernel_x weight file */
    char kernel_y_file[256];    /**< Path to kernel_y weight file */

    /* Buffer configuration */
    BufferType cl_buffer_type[MAX_CUSTOM_BUFFERS]; /**< Buffer types */
    size_t cl_buffer_size[MAX_CUSTOM_BUFFERS];     /**< Buffer sizes */
    int num_buffers;            /**< Number of configured buffers */
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
