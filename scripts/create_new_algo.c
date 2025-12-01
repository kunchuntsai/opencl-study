/**
 * @file create_new_algo.c
 * @brief Create new algorithm template for OpenCL framework
 *
 * Compile:
 *   gcc -o scripts/create_new_algo scripts/create_new_algo.c -Wall -O2
 *
 * Usage:
 *   ./scripts/create_new_algo <algorithm_name>
 *
 * Example:
 *   ./scripts/create_new_algo resize
 *
 * This will create:
 * - src/<algo>/c_ref/<algo>_ref.c (C reference implementation)
 * - src/<algo>/cl/<algo>0.cl (OpenCL kernel)
 * - config/<algo>.ini (Configuration file)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_PATH 512
#define MAX_NAME 128
#define MAX_CONTENT 16384

/* Forward declarations */
static int validate_algo_name(const char* name);
static void normalize_algo_name(const char* input, char* output);
static void to_uppercase(const char* input, char* output);
static int check_file_exists(const char* path);
static int create_directory(const char* path);
static int create_directories_recursive(const char* path);
static int write_file(const char* path, const char* content);
static void generate_c_ref_template(const char* algo_name, const char* algo_name_upper, char* output);
static void generate_opencl_kernel_template(const char* algo_name, const char* algo_name_upper, char* output);
static void generate_config_template(const char* algo_name, const char* algo_name_upper, char* output);

int main(int argc, char* argv[]) {
    char algo_name[MAX_NAME];
    char algo_name_c[MAX_NAME];
    char algo_name_upper[MAX_NAME];
    char content[MAX_CONTENT];
    char c_ref_dir[MAX_PATH];
    char cl_dir[MAX_PATH];
    char c_ref_file[MAX_PATH];
    char cl_file[MAX_PATH];
    char config_file[MAX_PATH];
    int has_errors = 0;

    if (argc != 2) {
        printf("Usage: %s <algorithm_name>\n", argv[0]);
        printf("\n");
        printf("Example:\n");
        printf("  %s resize\n", argv[0]);
        printf("  %s sobel_edge\n", argv[0]);
        printf("\n");
        return 1;
    }

    /* Get and validate algorithm name */
    strncpy(algo_name, argv[1], sizeof(algo_name) - 1);
    algo_name[sizeof(algo_name) - 1] = '\0';

    /* Convert to lowercase */
    for (int i = 0; algo_name[i]; i++) {
        algo_name[i] = (char)tolower((unsigned char)algo_name[i]);
    }

    if (!validate_algo_name(algo_name)) {
        fprintf(stderr, "Error: Algorithm name '%s' contains invalid characters.\n", algo_name);
        fprintf(stderr, "Use only letters, numbers, underscores, and hyphens.\n");
        return 1;
    }

    /* Normalize algorithm name (replace hyphens with underscores for C identifiers) */
    normalize_algo_name(algo_name, algo_name_c);
    to_uppercase(algo_name_c, algo_name_upper);

    /* Build paths */
    snprintf(c_ref_dir, sizeof(c_ref_dir), "src/%s/c_ref", algo_name);
    snprintf(cl_dir, sizeof(cl_dir), "src/%s/cl", algo_name);
    snprintf(c_ref_file, sizeof(c_ref_file), "%s/%s_ref.c", c_ref_dir, algo_name_c);
    snprintf(cl_file, sizeof(cl_file), "%s/%s0.cl", cl_dir, algo_name_c);
    snprintf(config_file, sizeof(config_file), "config/%s.ini", algo_name);

    /* Check if files already exist */
    if (check_file_exists(c_ref_file)) {
        fprintf(stderr, "Error: Algorithm '%s' already exists!\n", algo_name);
        fprintf(stderr, "File already exists: %s\n", c_ref_file);
        has_errors = 1;
    }
    if (check_file_exists(cl_file)) {
        if (!has_errors) {
            fprintf(stderr, "Error: Algorithm '%s' already exists!\n", algo_name);
        }
        fprintf(stderr, "File already exists: %s\n", cl_file);
        has_errors = 1;
    }
    if (check_file_exists(config_file)) {
        if (!has_errors) {
            fprintf(stderr, "Error: Algorithm '%s' already exists!\n", algo_name);
        }
        fprintf(stderr, "File already exists: %s\n", config_file);
        has_errors = 1;
    }

    if (has_errors) {
        return 1;
    }

    /* Create directories */
    printf("Creating algorithm template for: %s\n\n", algo_name);

    if (create_directories_recursive(c_ref_dir) != 0) {
        fprintf(stderr, "Error: Failed to create directory: %s\n", c_ref_dir);
        return 1;
    }
    printf("✓ Created directory: %s\n", c_ref_dir);

    if (create_directories_recursive(cl_dir) != 0) {
        fprintf(stderr, "Error: Failed to create directory: %s\n", cl_dir);
        return 1;
    }
    printf("✓ Created directory: %s\n", cl_dir);

    /* Ensure config directory exists */
    create_directory("config");

    /* Generate and write C reference implementation */
    generate_c_ref_template(algo_name_c, algo_name_upper, content);
    if (write_file(c_ref_file, content) != 0) {
        fprintf(stderr, "Error: Failed to write file: %s\n", c_ref_file);
        return 1;
    }
    printf("✓ Created file: %s\n", c_ref_file);

    /* Generate and write OpenCL kernel */
    generate_opencl_kernel_template(algo_name_c, algo_name_upper, content);
    if (write_file(cl_file, content) != 0) {
        fprintf(stderr, "Error: Failed to write file: %s\n", cl_file);
        return 1;
    }
    printf("✓ Created file: %s\n", cl_file);

    /* Generate and write config file */
    generate_config_template(algo_name, algo_name_upper, content);
    if (write_file(config_file, content) != 0) {
        fprintf(stderr, "Error: Failed to write file: %s\n", config_file);
        return 1;
    }
    printf("✓ Created file: %s\n", config_file);

    /* Print success message and next steps */
    printf("\n");
    printf("======================================================================\n");
    printf("Algorithm template created successfully!\n");
    printf("======================================================================\n");
    printf("\n");
    printf("Next steps:\n");
    printf("1. Implement the algorithm in: %s\n", c_ref_file);
    printf("2. Implement the OpenCL kernel in: %s\n", cl_file);
    printf("3. Configure parameters in: %s\n", config_file);
    printf("4. Create test data directory: test_data/%s/\n", algo_name);
    printf("5. Generate test input: python3 scripts/generate_test_image.py\n");
    printf("6. Rebuild the project: ./scripts/build.sh\n");
    printf("7. Run your algorithm: ./build/opencl_host %s 0\n", algo_name);
    printf("\n");
    printf("See docs/ADD_NEW_ALGO.md for detailed implementation guide.\n");

    return 0;
}

static int validate_algo_name(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    for (int i = 0; name[i]; i++) {
        char c = name[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '-') {
            return 0;
        }
    }

    return 1;
}

static void normalize_algo_name(const char* input, char* output) {
    int i;
    for (i = 0; input[i]; i++) {
        output[i] = (input[i] == '-') ? '_' : input[i];
    }
    output[i] = '\0';
}

static void to_uppercase(const char* input, char* output) {
    int i;
    for (i = 0; input[i]; i++) {
        output[i] = (char)toupper((unsigned char)input[i]);
    }
    output[i] = '\0';
}

static int check_file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static int create_directory(const char* path) {
#ifdef _WIN32
    return mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

static int create_directories_recursive(const char* path) {
    char tmp[MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (create_directory(tmp) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }

    if (create_directory(tmp) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

static int write_file(const char* path, const char* content) {
    FILE* fp = fopen(path, "w");
    if (fp == NULL) {
        return -1;
    }

    if (fputs(content, fp) == EOF) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

static void generate_c_ref_template(const char* algo_name, const char* algo_name_upper, char* output) {
    snprintf(output, MAX_CONTENT,
"#include \"../../utils/safe_ops.h\"\n"
"#include \"../../utils/op_interface.h\"\n"
"#include \"../../utils/op_registry.h\"\n"
"#include \"../../utils/verify.h\"\n"
"#include <stddef.h>\n"
"#include <stdio.h>\n"
"\n"
"/**\n"
" * @brief %s reference implementation\n"
" *\n"
" * CPU implementation of %s algorithm.\n"
" * This serves as the ground truth for verifying GPU output.\n"
" *\n"
" * @param[in] params Operation parameters containing:\n"
" *   - input: Input image buffer\n"
" *   - output: Output image buffer\n"
" *   - src_width, src_height: Source dimensions\n"
" *   - dst_width, dst_height: Destination dimensions\n"
" *   - custom_buffers: Optional custom buffers (NULL if none)\n"
" */\n"
"void %s_ref(const OpParams* params) {\n"
"    int y;\n"
"    int x;\n"
"    int width;\n"
"    int height;\n"
"    unsigned char* input;\n"
"    unsigned char* output;\n"
"    int total_pixels;\n"
"\n"
"    if (params == NULL) {\n"
"        return;\n"
"    }\n"
"\n"
"    /* Extract parameters */\n"
"    input = params->input;\n"
"    output = params->output;\n"
"    width = params->src_width;\n"
"    height = params->src_height;\n"
"\n"
"    if ((input == NULL) || (output == NULL) || (width <= 0) || (height <= 0)) {\n"
"        return;\n"
"    }\n"
"\n"
"    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */\n"
"    if (!safe_mul_int(width, height, &total_pixels)) {\n"
"        return; /* Overflow detected */\n"
"    }\n"
"\n"
"    /* TODO: Implement your algorithm here */\n"
"    /* Example: Simple copy operation */\n"
"    for (y = 0; y < height; y++) {\n"
"        for (x = 0; x < width; x++) {\n"
"            int index = y * width + x;\n"
"            if (index < total_pixels) {\n"
"                output[index] = input[index];\n"
"            }\n"
"        }\n"
"    }\n"
"}\n"
"\n"
"/**\n"
" * @brief Verify GPU result against reference\n"
" *\n"
" * Compares GPU output with reference implementation output.\n"
" *\n"
" * @param[in] params Operation parameters containing gpu_output and ref_output\n"
" * @param[out] max_error Maximum absolute difference found\n"
" * @return 1 if verification passed, 0 if failed\n"
" */\n"
"int %s_verify(const OpParams* params, float* max_error) {\n"
"    if (params == NULL) {\n"
"        return 0;\n"
"    }\n"
"\n"
"    /* For exact match verification (e.g., for morphological operations) */\n"
"    /* Uncomment this if you need exact matching:\n"
"    int result = verify_exact_match(params->gpu_output, params->ref_output,\n"
"                                    params->dst_width, params->dst_height, 0);\n"
"    if (max_error != NULL) {\n"
"        *max_error = (result == 1) ? 0.0f : 1.0f;\n"
"    }\n"
"    return result;\n"
"    */\n"
"\n"
"    /* For floating-point algorithms with tolerance */\n"
"    /* Allow small differences due to rounding (adjust tolerance as needed) */\n"
"    return verify_with_tolerance(params->gpu_output, params->ref_output,\n"
"                                params->dst_width, params->dst_height,\n"
"                                1.0f,      /* max_pixel_diff: 1 intensity level */\n"
"                                0.001f,    /* max_error_ratio: 0.1%% of pixels can differ */\n"
"                                max_error);\n"
"}\n"
"\n"
"/**\n"
" * @brief Set kernel arguments\n"
" *\n"
" * Sets all kernel arguments in the order expected by the OpenCL kernel.\n"
" * Must match the kernel signature exactly.\n"
" *\n"
" * @param[in] kernel OpenCL kernel handle\n"
" * @param[in] input_buf Input buffer\n"
" * @param[in] output_buf Output buffer\n"
" * @param[in] params Operation parameters\n"
" * @return 0 on success, -1 on error\n"
" */\n"
"int %s_set_kernel_args(cl_kernel kernel,\n"
"                                cl_mem input_buf,\n"
"                                cl_mem output_buf,\n"
"                                const OpParams* params) {\n"
"    cl_uint arg_idx = 0U;\n"
"\n"
"    if ((kernel == NULL) || (params == NULL)) {\n"
"        return -1;\n"
"    }\n"
"\n"
"    /* Standard arguments: input, output, width, height */\n"
"    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {\n"
"        return -1;\n"
"    }\n"
"    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {\n"
"        return -1;\n"
"    }\n"
"    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {\n"
"        return -1;\n"
"    }\n"
"    if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {\n"
"        return -1;\n"
"    }\n"
"\n"
"    /* If your algorithm uses custom buffers, uncomment and modify this:\n"
"    if (params->custom_buffers == NULL) {\n"
"        (void)fprintf(stderr, \"Error: %s requires custom buffers\\n\");\n"
"        return -1;\n"
"    }\n"
"    CustomBuffers* custom = params->custom_buffers;\n"
"    if (custom->count < 1) {\n"
"        (void)fprintf(stderr, \"Error: %s requires at least 1 custom buffer\\n\");\n"
"        return -1;\n"
"    }\n"
"\n"
"    // Set custom buffer arguments\n"
"    if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &custom->buffers[0].buffer) != CL_SUCCESS) {\n"
"        return -1;\n"
"    }\n"
"    */\n"
"\n"
"    return 0;\n"
"}\n"
"\n"
"/*\n"
" * NOTE: Registration of this algorithm happens in auto_registry.c\n"
" * See src/utils/auto_registry.c for the registration code.\n"
" */\n",
        algo_name_upper, algo_name, algo_name, algo_name, algo_name,
        algo_name_upper, algo_name_upper);
}

static void generate_opencl_kernel_template(const char* algo_name, const char* algo_name_upper, char* output) {
    snprintf(output, MAX_CONTENT,
"/**\n"
" * @file %s0.cl\n"
" * @brief %s OpenCL kernel implementation\n"
" *\n"
" * TODO: Add algorithm description here\n"
" *\n"
" * Kernel arguments:\n"
" * @param input  Input image buffer\n"
" * @param output Output image buffer\n"
" * @param width  Image width in pixels\n"
" * @param height Image height in pixels\n"
" */\n"
"\n"
"__kernel void %s(__global const uchar* input,\n"
"                         __global uchar* output,\n"
"                         int width,\n"
"                         int height) {\n"
"    int x = get_global_id(0);\n"
"    int y = get_global_id(1);\n"
"\n"
"    /* Boundary check */\n"
"    if (x >= width || y >= height) return;\n"
"\n"
"    int index = y * width + x;\n"
"\n"
"    /* TODO: Implement your algorithm here */\n"
"    /* Example: Simple copy operation */\n"
"    output[index] = input[index];\n"
"}\n",
        algo_name, algo_name_upper, algo_name);
}

static void generate_config_template(const char* algo_name, const char* algo_name_upper, char* output) {
    snprintf(output, MAX_CONTENT,
"# %s Algorithm Configuration\n"
"# TODO: Add algorithm description here\n"
"# Note: op_id is auto-derived from filename (%s.ini -> op_id = %s)\n"
"\n"
"[image]\n"
"input = test_data/%s/input.bin\n"
"output = test_data/%s/output.bin\n"
"src_width = 1920\n"
"src_height = 1080\n"
"dst_width = 1920\n"
"dst_height = 1080\n"
"\n"
"# Variant 0: Basic implementation using standard OpenCL API\n"
"[kernel.v0]\n"
"host_type = standard   # Options: \"standard\" (default) or \"cl_extension\"\n"
"kernel_file = src/%s/cl/%s0.cl\n"
"kernel_function = %s\n"
"work_dim = 2\n"
"global_work_size = 1920,1088\n"
"local_work_size = 16,16\n"
"\n"
"# Optional: Add custom buffers if needed\n"
"# Example: Custom buffer for algorithm-specific data\n"
"# [buffer.custom_data]\n"
"# type = READ_ONLY\n"
"# data_type = float\n"
"# num_elements = 100\n"
"# source_file = test_data/%s/custom_data.bin\n"
"\n"
"# Optional: Add more kernel variants\n"
"# [kernel.v1]\n"
"# host_type = cl_extension\n"
"# kernel_file = src/%s/cl/%s1.cl\n"
"# kernel_function = %s_optimized\n"
"# work_dim = 2\n"
"# global_work_size = 1920,1088\n"
"# local_work_size = 16,16\n",
        algo_name_upper, algo_name, algo_name,
        algo_name, algo_name,
        algo_name, algo_name, algo_name,
        algo_name,
        algo_name, algo_name, algo_name);
}
