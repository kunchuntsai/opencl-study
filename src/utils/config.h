/**
 * @file config.h
 * @brief Configuration file parser for OpenCL framework
 *
 * Parses JSON configuration files containing:
 * - Image processing parameters (dimensions, file paths)
 * - Algorithm selection (op_id)
 * - Kernel variant configurations (work sizes, kernel files)
 * - Buffer configurations (types, sizes)
 *
 * Configuration file format (JSON):
 * {
 *   "input": { "input_image_id": "image_1" },
 *   "output": { "output_image_id": "output_1" },
 *   "verification": { "tolerance": 0, "error_rate_threshold": 0 },
 *   "kernels": {
 *     "v0": {
 *       "kernel_file": "path/to/kernel.cl",
 *       "kernel_function": "function_name",
 *       "work_dim": 2,
 *       "global_work_size": [1920, 1080],
 *       "local_work_size": [16, 16],
 *       "kernel_args": [
 *         {"i_buffer": ["uchar", "src"]},
 *         {"o_buffer": ["uchar", "dst"]},
 *         {"param": ["int", "src_width"]}
 *       ]
 *     }
 *   }
 * }
 *
 * MISRA C 2023 Compliance:
 * - Rule 21.7: Uses strtok_r (reentrant) instead of strtok
 * - Rule 21.8: Uses SafeStrtol instead of atoi
 * - Rule 8.14: All structures properly tagged
 */

#pragma once

#include <stddef.h>

#include "op_interface.h" /* For MAX_CUSTOM_BUFFERS and CustomBuffers type */

/** Maximum number of kernel configurations per algorithm */
#define MAX_KERNEL_CONFIGS 32

/** Maximum number of input images */
#define MAX_INPUT_IMAGES 16

/** Maximum number of output images */
#define MAX_OUTPUT_IMAGES 16

/* Note: HostType and BufferType are defined in utils/op_interface.h */

/**
 * @brief Input image configuration
 *
 * Contains parameters for a single input image, including file path
 * and image dimensions.
 */
typedef struct {
    char input_path[256]; /**< Path to input image file */
    int src_width;        /**< Source image width in pixels */
    int src_height;       /**< Source image height in pixels */
    int src_channels;     /**< Number of channels (e.g., 3 for RGB) */
    int src_stride;       /**< Stride in bytes (may differ from width * channels) */
} InputImageConfig;

/**
 * @brief Output image configuration
 *
 * Contains parameters for a single output image, including file path
 * and image dimensions.
 */
typedef struct {
    char output_path[256]; /**< Path to output image file */
    int dst_width;         /**< Destination image width in pixels */
    int dst_height;        /**< Destination image height in pixels */
    int dst_channels;      /**< Number of channels (e.g., 3 for RGB) */
    int dst_stride;        /**< Stride in bytes (may differ from width * channels) */
} OutputImageConfig;

/* Note: MAX_CUSTOM_BUFFERS is defined in utils/op_interface.h */

/** Data type enumeration for buffer elements */
typedef enum {
    DATA_TYPE_NONE = 0,
    DATA_TYPE_FLOAT, /**< 32-bit floating point (4 bytes) */
    DATA_TYPE_UCHAR, /**< 8-bit unsigned char (1 byte) */
    DATA_TYPE_INT,   /**< 32-bit signed integer (4 bytes) */
    DATA_TYPE_SHORT  /**< 16-bit signed integer (2 bytes) */
} DataType;

/** Maximum number of kernel arguments */
#define MAX_KERNEL_ARGS 32

/** Maximum number of fields in a struct argument */
#define MAX_STRUCT_FIELDS 16

/** Kernel argument type */
typedef enum {
    KERNEL_ARG_TYPE_NONE = 0,
    KERNEL_ARG_TYPE_BUFFER_INPUT,  /**< Input buffer (cl_mem) */
    KERNEL_ARG_TYPE_BUFFER_OUTPUT, /**< Output buffer (cl_mem) */
    KERNEL_ARG_TYPE_BUFFER_CUSTOM, /**< Custom buffer (cl_mem) by name */
    KERNEL_ARG_TYPE_SCALAR_INT,    /**< Integer scalar (int) */
    KERNEL_ARG_TYPE_SCALAR_FLOAT,  /**< Float scalar (float) */
    KERNEL_ARG_TYPE_SCALAR_SIZE,   /**< Size_t scalar (size_t) */
    KERNEL_ARG_TYPE_STRUCT,        /**< Struct packed from scalars */
} KernelArgType;

/**
 * @brief Kernel argument descriptor
 *
 * Describes a single kernel argument with its type, data type, and name.
 * Arguments can be buffers (input, output, or custom), scalars, or structs.
 *
 * JSON format: {"key": ["data_type", "name"]} or {"key": ["data_type", "name", size]}
 * - i_buffer: Input buffer  (e.g., {"i_buffer": ["uchar", "src"]})
 * - o_buffer: Output buffer (e.g., {"o_buffer": ["uchar", "dst"]})
 * - buffer:   Custom buffer (e.g., {"buffer": ["uchar", "tmp", 45000]})
 * - param:    Scalar param  (e.g., {"param": ["int", "src_width"]})
 * - struct:   Packed struct (e.g., {"struct": ["field1", "field2", ...]})
 *             Fields reference scalars defined in the "scalars" section
 */
typedef struct {
    KernelArgType arg_type; /**< Type of argument (buffer or scalar) */
    DataType data_type;     /**< Data type of the argument (uchar, int, float, etc.) */
    char source_name[64];   /**< Source name for the argument value:
                                 - For buffers: buffer name (e.g., "src", "dst", "weights")
                                 - For scalars: param name (e.g., "src_width", "dst_height")
                             */
    size_t buffer_size;     /**< Buffer size in bytes (0 if not specified, for buffer types only) */

    /* Struct fields (for KERNEL_ARG_TYPE_STRUCT only) */
    char struct_fields[MAX_STRUCT_FIELDS][64]; /**< Array of scalar names to pack into struct */
    int struct_field_count;                    /**< Number of fields in struct */
} KernelArgDescriptor;

/**
 * @brief Kernel configuration for a specific variant
 *
 * Contains all parameters needed to execute an OpenCL kernel variant,
 * including work group dimensions and kernel source location.
 */
typedef struct KernelConfig {
    char variant_id[32];        /**< Variant identifier (e.g., "v0", "v1") */
    char description[128];      /**< Human-readable description (e.g., "standard OpenCL") */
    char kernel_file[256];      /**< Path to OpenCL kernel source file */
    char kernel_function[64];   /**< Kernel function name to execute */
    int work_dim;               /**< Work dimensions (1, 2, or 3) */
    size_t global_work_size[3]; /**< Global work size for each dimension */
    size_t local_work_size[3];  /**< Local work group size for each dimension */
    HostType host_type;         /**< Host API type (standard or cl_extension) */
    char kernel_option[256];    /**< User-specified kernel build options (e.g.,
                                   "-cl-fast-relaxed-math"). System appends platform defines:
                                   -DHOST_TYPE=N -I<CL_INCLUDE_DIR> (absolute path) */
    int kernel_variant;         /**< Kernel signature variant auto-derived from variant_id
                                   (v0->0, v1->1, etc.) */
    KernelArgDescriptor kernel_args[MAX_KERNEL_ARGS]; /**< Array of kernel argument descriptors */
    int kernel_arg_count;                             /**< Number of kernel arguments configured */
} KernelConfig;

/**
 * @brief Custom buffer configuration
 *
 * Describes a custom OpenCL buffer that will be created and passed to the
 * kernel. Buffers are set as kernel arguments sequentially after input/output
 * buffers. Order in config file determines kernel argument order: arg 0: input
 * (standard) arg 1: output (standard) arg 2: first custom buffer arg 3: second
 * custom buffer
 *   ...
 *
 * Buffers can either be:
 * - File-backed: Loaded from source_file, sized by data_type * num_elements
 * - Empty: Allocated with size_bytes, no initialization
 */
typedef struct {
    char name[64];   /**< Buffer name (from [buffer.NAME] section) */
    BufferType type; /**< Buffer access type (READ_ONLY, WRITE_ONLY, READ_WRITE) */

    /* File-backed buffer fields */
    char source_file[256]; /**< Path to data file (empty string if not
                              file-backed) */
    DataType data_type;    /**< Element data type (for file-backed buffers) */
    int num_elements;      /**< Number of elements (for file-backed buffers) */

    /* Empty buffer fields */
    size_t size_bytes; /**< Direct size specification (for empty buffers) */
} CustomBufferConfig;

/**
 * @brief Scalar argument configuration
 *
 * Describes a scalar argument to be passed to the kernel (e.g., filter_width,
 * filter_height). Supports int, float, and size_t types.
 *
 * Config file format:
 * [scalar.filter_width]
 * type = int
 * value = 5
 *
 * [scalar.sigma]
 * type = float
 * value = 1.5
 */
typedef struct {
    char name[64];   /**< Argument name (from [scalar.NAME] section) */
    ScalarType type; /**< Value type (int, float, or size_t) */
    union {
        int int_value;     /**< Integer value */
        float float_value; /**< Float value */
        size_t size_value; /**< Size value */
    } value;
} ScalarArgConfig;

/** Maximum number of scalar arguments */
#define MAX_SCALAR_ARGS 32

/**
 * @brief Golden sample source type
 *
 * Specifies how to obtain the golden sample for verification:
 * - GOLDEN_SOURCE_C_REF: Generate from C reference implementation (default)
 * - GOLDEN_SOURCE_FILE: Load from pre-computed golden.bin file
 */
typedef enum {
    GOLDEN_SOURCE_C_REF = 0, /**< Generate golden from C reference (default) */
    GOLDEN_SOURCE_FILE       /**< Load golden from file (skip c_ref) */
} GoldenSourceType;

/**
 * @brief Verification configuration
 *
 * Specifies how GPU output should be verified against reference output.
 * This replaces algorithm-specific verify_result callbacks with config-driven
 * verification using generic library functions.
 *
 * Golden sample source:
 * - If golden_source = c_ref (default): Run C reference implementation to generate golden
 * - If golden_source = file: Load golden from golden_file path (skip c_ref execution)
 */
typedef struct {
    float tolerance; /**< Max per-pixel difference allowed (e.g., 0 for exact, 1 for ±1) */
    float error_rate_threshold; /**< Max fraction of pixels that can exceed tolerance (e.g., 0.001 =
                                   0.1%) */
    GoldenSourceType golden_source; /**< Source of golden sample (c_ref or file) */
    char golden_file[256];          /**< Path to golden.bin file (when golden_source = file) */
} VerificationConfig;

/**
 * @brief Complete configuration parsed from config file
 *
 * Contains all settings needed to run the OpenCL image processing
 * framework, including image parameters and all kernel variants.
 */
typedef struct {
    char op_id[32]; /**< Algorithm identifier (e.g., "dilate3x3") */

    /* Input images configuration (from config/inputs.json) */
    InputImageConfig input_images[MAX_INPUT_IMAGES]; /**< Array of input image configurations */
    int input_image_count;                           /**< Number of input images configured */
    char input_image_id[64]; /**< Which input image to use (e.g., "image_1", "image_2") */

    /* Output images configuration (from config/outputs.json) */
    OutputImageConfig output_images[MAX_OUTPUT_IMAGES]; /**< Array of output image configurations */
    int output_image_count;                             /**< Number of output images configured */
    char output_image_id[64]; /**< Which output image to use (e.g., "output_1", "output_2") */

    /* Kernel configurations */
    int num_kernels;                          /**< Number of kernel variants configured */
    KernelConfig kernels[MAX_KERNEL_CONFIGS]; /**< Array of kernel configurations */

    /* Custom buffer configuration */
    CustomBufferConfig custom_buffers[MAX_CUSTOM_BUFFERS]; /**< Custom buffer configurations */
    int custom_buffer_count; /**< Number of custom buffers configured */

    /* Scalar argument configuration */
    ScalarArgConfig scalar_args[MAX_SCALAR_ARGS]; /**< Scalar argument configurations */
    int scalar_arg_count;                         /**< Number of scalar arguments configured */

    /* Verification configuration */
    VerificationConfig verification; /**< Verification settings (tolerance, error rate) */
} Config;

/**
 * @brief Parse configuration file
 *
 * Reads and parses a JSON configuration file into a Config structure.
 * Supports input, output, verification, scalars, buffers, and kernels sections.
 *
 * @param[in] filename Path to configuration file
 * @param[out] config Configuration structure to populate
 * @return 0 on success, -1 on error
 */
int ParseConfig(const char* filename, Config* config);

/**
 * @brief Parse input images configuration file
 *
 * Reads and parses a JSON inputs configuration file with multiple
 * image_N entries, populating the input_images array in Config.
 *
 * @param[in] filename Path to inputs configuration file (e.g., config/inputs.json)
 * @param[out] config Configuration structure to populate with input images
 * @return 0 on success, -1 on error
 */
int ParseInputsConfig(const char* filename, Config* config);

/**
 * @brief Parse output images configuration file
 *
 * Reads and parses a JSON outputs configuration file with multiple
 * output_N entries, populating the output_images array in Config.
 *
 * @param[in] filename Path to outputs configuration file (e.g., config/outputs.json)
 * @param[out] config Configuration structure to populate with output images
 * @return 0 on success, -1 on error
 */
int ParseOutputsConfig(const char* filename, Config* config);

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
int GetOpVariants(const Config* config, const char* op_id, KernelConfig* variants[], int* count);

/**
 * @brief Resolve config path from algorithm name
 *
 * Converts algorithm name to config path:
 * Algorithm name (e.g., "dilate3x3") -> "config/dilate3x3.json"
 *
 * Verifies that the resolved file exists.
 *
 * @param[in] input User input (algorithm name)
 * @param[out] output Resolved config file path
 * @param[in] output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int ResolveConfigPath(const char* input, char* output, size_t output_size);

/**
 * @brief Extract op_id from config file path
 *
 * Extracts the base filename without extension to use as op_id.
 * Example: "config/dilate3x3.json" -> "dilate3x3"
 *
 * @param[in] config_path Path to config file
 * @param[out] op_id Extracted algorithm identifier
 * @param[in] op_id_size Size of op_id buffer
 * @return 0 on success, -1 on error
 */
int ExtractOpIdFromPath(const char* config_path, char* op_id, size_t op_id_size);
