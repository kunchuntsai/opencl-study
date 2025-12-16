#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include internal headers with full type definitions */
#include "op_registry.h"
#include "platform/cache_manager.h"
#include "platform/opencl_utils.h"
#include "utils/config.h"
#include "utils/safe_ops.h"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_PATH_LENGTH 512
#define MAX_IMAGE_SIZE (4096 * 4096)

/* Configuration file paths */
#define CONFIG_INPUTS_PATH "config/inputs.json"
#define CONFIG_OUTPUTS_PATH "config/outputs.json"

static unsigned char gpu_output_buffer[MAX_IMAGE_SIZE];
static unsigned char ref_output_buffer[MAX_IMAGE_SIZE];

/* Forward declarations */
void RunAlgorithm(const Algorithm* algo, const KernelConfig* kernel_cfg, const Config* config,
                  OpenCLEnv* env, unsigned char* gpu_output_buffer,
                  unsigned char* ref_output_buffer);

void AutoRegisterAlgorithms(void);

static int SelectAlgorithmAndVariant(const Config* config, int provided_variant_index,
                                     Algorithm** selected_algo, KernelConfig** variants,
                                     int* variant_count, int* selected_variant_index);

int main(int argc, char** argv) {
    Config config;

    /* Register all algorithms */
    AutoRegisterAlgorithms();
    OpenCLEnv env;
    int variant_index;
    Algorithm* algo;
    KernelConfig* variants[MAX_KERNEL_CONFIGS];
    int variant_count;
    int parse_result;
    int opencl_result;
    long temp_index;
    char config_path[MAX_PATH_LENGTH];
    const char* config_input;

    /* Check for help flags */
    if ((argc == 2) && ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0) ||
                        (strcmp(argv[1], "help") == 0))) {
        (void)printf("Usage: %s <algorithm> [variant_index]\n", argv[0]);
        (void)printf("\n");
        (void)printf("Available Algorithms:\n");
        ListAlgorithms();
        return 0;
    }

    /* Check command line arguments */
    if (argc > 3) {
        (void)fprintf(stderr, "Error: Too many arguments\n");
        (void)fprintf(stderr, "Usage: %s <algorithm> [variant_index]\n", argv[0]);
        (void)fprintf(stderr, "Run '%s --help' for more information\n", argv[0]);
        return 1;
    }

    /* Parse command line arguments - MISRA-C:2023 Rule 21.8: Avoid atoi() */
    if (argc < 2) {
        (void)fprintf(stderr, "Error: Algorithm name required\n");
        (void)fprintf(stderr, "Usage: %s <algorithm> [variant_index]\n", argv[0]);
        (void)fprintf(stderr, "\nAvailable algorithms:\n");
        ListAlgorithms();
        (void)fprintf(stderr, "\nRun '%s --help' for more information\n", argv[0]);
        return 1;
    }

    /* Argument 1: Algorithm name (required) */
    config_input = argv[1];

    /* Argument 2: Variant index (optional, default: interactive) */
    if (argc == 3) {
        if (!SafeStrtol(argv[2], &temp_index)) {
            (void)fprintf(stderr, "Invalid variant index: %s\n", argv[2]);
            return 1;
        }
        variant_index = (int)temp_index;
    } else {
        variant_index = -1;
    }

    /* Resolve algorithm name to config path (config/<name>.json) */
    if (ResolveConfigPath(config_input, config_path, sizeof(config_path)) != 0) {
        (void)fprintf(stderr, "Failed to resolve config path: %s\n", config_input);
        return 1;
    }

    /* 1a. Parse input images configuration */
    parse_result = ParseInputsConfig(CONFIG_INPUTS_PATH, &config);
    if (parse_result != 0) {
        (void)fprintf(stderr, "Failed to parse %s\n", CONFIG_INPUTS_PATH);
        return 1;
    }

    /* 1b. Parse output images configuration */
    parse_result = ParseOutputsConfig(CONFIG_OUTPUTS_PATH, &config);
    if (parse_result != 0) {
        (void)fprintf(stderr, "Failed to parse %s\n", CONFIG_OUTPUTS_PATH);
        return 1;
    }

    /* 1c. Parse algorithm configuration */
    parse_result = ParseConfig(config_path, &config);
    if (parse_result != 0) {
        (void)fprintf(stderr, "Failed to parse %s\n", config_path);
        return 1;
    }

    /* Auto-derive op_id from filename if not specified in config */
    if ((config.op_id[0] == '\0') || (strcmp(config.op_id, "config") == 0)) {
        if (ExtractOpIdFromPath(config_path, config.op_id, sizeof(config.op_id)) != 0) {
            (void)fprintf(stderr, "Warning: Could not derive op_id from filename\n");
        }
    }

    /* 3. Select algorithm and kernel variant (with user interaction if needed) */
    if (SelectAlgorithmAndVariant(&config, variant_index, &algo, variants, &variant_count,
                                  &variant_index) != 0) {
        return 1;
    }

    /* 2. Initialize OpenCL */
    (void)printf("=== OpenCL Initialization ===\n");
    opencl_result = OpenclInit(&env);
    if (opencl_result != 0) {
        (void)fprintf(stderr, "Failed to initialize OpenCL\n");
        return 1;
    }

    /* 5. Initialize cache directories for this algorithm */
    if (CacheInit(algo->id, variants[variant_index]->variant_id) != 0) {
        (void)fprintf(stderr, "Warning: Failed to initialize cache directories for %s\n", algo->id);
    }

    /* 7. Run algorithm */
    (void)printf("\n=== Running %s (variant: %s) ===\n", algo->name,
                 variants[variant_index]->variant_id);
    RunAlgorithm(algo, variants[variant_index], &config, &env, gpu_output_buffer,
                 ref_output_buffer);

    /* Cleanup */
    OpenclCleanup(&env);
    return 0;
}

/**
 * @brief Select algorithm and kernel variant (with user interaction if needed)
 *
 * This function:
 * 1. Displays available algorithms and selects the one from config
 * 2. Gets and displays kernel variants for the selected algorithm
 * 3. Selects variant (from command line parameter or interactive prompt)
 * 4. Validates the selection
 *
 * @param[in] config Configuration containing op_id
 * @param[in] provided_variant_index Variant index from command line, or -1 for
 * interactive
 * @param[out] selected_algo Pointer to receive the selected algorithm
 * @param[out] variants Array to receive variant configurations
 * @param[out] variant_count Number of variants found
 * @param[out] selected_variant_index The selected variant index
 * @return 0 on success, -1 on error
 */
static int SelectAlgorithmAndVariant(const Config* config, int provided_variant_index,
                                     Algorithm** selected_algo, KernelConfig** variants,
                                     int* variant_count, int* selected_variant_index) {
    Algorithm* algo;
    int get_variants_result;
    int i;
    char input_buffer[32];
    char* newline_pos;
    long temp_index;

    if ((config == NULL) || (selected_algo == NULL) || (variants == NULL) ||
        (variant_count == NULL) || (selected_variant_index == NULL)) {
        return -1;
    }

    /* Step 1: Find selected algorithm based on config.op_id */
    algo = FindAlgorithm(config->op_id);
    if (algo == NULL) {
        (void)fprintf(stderr, "Error: Algorithm '%s' (from config) not found\n", config->op_id);
        (void)fprintf(stderr, "\n=== Available Algorithms ===\n");
        ListAlgorithms();
        return -1;
    }

    /* Step 2: Get kernel variants for selected algorithm */
    get_variants_result = GetOpVariants(config, algo->id, variants, variant_count);
    if ((get_variants_result != 0) || (*variant_count == 0)) {
        (void)fprintf(stderr, "No kernel variants configured for %s\n", algo->name);
        return -1;
    }

    /* Step 3: Display selected algorithm and its variants */
    (void)printf("\n=== Algorithm: %s ===\n", algo->name);
    (void)printf("Available variants:\n");
    for (i = 0; i < *variant_count; i++) {
        (void)printf("  %d - %s\n", i, variants[i]->variant_id);
    }
    (void)printf("\n");

    /* Step 5: Select variant (from command line or interactive prompt) */
    if (provided_variant_index >= 0) {
        /* Variant index was provided via command line - validate it */
        *selected_variant_index = provided_variant_index;
        if ((*selected_variant_index < 0) || (*selected_variant_index >= *variant_count)) {
            (void)fprintf(stderr, "Error: Invalid variant index: %d (available: 0-%d)\n",
                          *selected_variant_index, *variant_count - 1);
            return -1;
        }
    } else {
        /* No variant index provided - prompt user interactively */
        (void)printf("Select variant (0-%d, default: 0): ", *variant_count - 1);
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            (void)fprintf(stderr, "Failed to read input\n");
            return -1;
        }

        /* Remove trailing newline if present */
        newline_pos = strchr(input_buffer, '\n');
        if (newline_pos != NULL) {
            *newline_pos = '\0';
        }

        /* If empty input, use default variant 0 */
        if (input_buffer[0] == '\0') {
            *selected_variant_index = 0;
        } else {
            if (!SafeStrtol(input_buffer, &temp_index)) {
                (void)fprintf(stderr, "Invalid variant selection\n");
                return -1;
            }
            *selected_variant_index = (int)temp_index;

            /* Validate range */
            if ((*selected_variant_index < 0) || (*selected_variant_index >= *variant_count)) {
                (void)fprintf(stderr, "Error: Invalid variant index: %d (available: 0-%d)\n",
                              *selected_variant_index, *variant_count - 1);
                return -1;
            }
        }
    }

    *selected_algo = algo;
    return 0;
}
