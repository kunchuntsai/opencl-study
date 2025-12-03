#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algorithm_runner.h"
#include "utils/config.h"
#include "utils/opencl_utils.h"
#include "utils/cache_manager.h"
#include "utils/op_registry.h"
#include "utils/safe_ops.h"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_PATH_LENGTH 512

static unsigned char gpu_output_buffer[MAX_IMAGE_SIZE];
static unsigned char ref_output_buffer[MAX_IMAGE_SIZE];

/* Forward declarations */
static int select_algorithm_and_variant(const Config* config, int provided_variant_index,
                                        Algorithm** selected_algo, KernelConfig** variants,
                                        int* variant_count, int* selected_variant_index);

int main(int argc, char** argv) {
    Config config;
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
    if ((argc == 2) && ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "help") == 0))) {
        (void)printf("Usage: %s <algorithm> [variant_index]\n", argv[0]);
        (void)printf("\n");
        (void)printf("Available Algorithms:\n");
        list_algorithms();
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
        list_algorithms();
        (void)fprintf(stderr, "\nRun '%s --help' for more information\n", argv[0]);
        return 1;
    }

    /* Argument 1: Algorithm name (required) */
    config_input = argv[1];

    /* Argument 2: Variant index (optional, default: interactive) */
    if (argc == 3) {
        if (!safe_strtol(argv[2], &temp_index)) {
            (void)fprintf(stderr, "Invalid variant index: %s\n", argv[2]);
            return 1;
        }
        variant_index = (int)temp_index;
    } else {
        variant_index = -1;
    }

    /* Resolve algorithm name to config path (config/<name>.ini) */
    if (resolve_config_path(config_input, config_path, sizeof(config_path)) != 0) {
        (void)fprintf(stderr, "Failed to resolve config path: %s\n", config_input);
        return 1;
    }

    /* 1. Parse configuration */
    parse_result = parse_config(config_path, &config);
    if (parse_result != 0) {
        (void)fprintf(stderr, "Failed to parse %s\n", config_path);
        return 1;
    }

    /* Auto-derive op_id from filename if not specified in config */
    if ((config.op_id[0] == '\0') || (strcmp(config.op_id, "config") == 0)) {
        if (extract_op_id_from_path(config_path, config.op_id, sizeof(config.op_id)) != 0) {
            (void)fprintf(stderr, "Warning: Could not derive op_id from filename\n");
        }
    }

    /* 3. Select algorithm and kernel variant (with user interaction if needed) */
    if (select_algorithm_and_variant(&config, variant_index, &algo, variants,
                                     &variant_count, &variant_index) != 0) {
        return 1;
    }

    /* 2. Initialize OpenCL */
    (void)printf("=== OpenCL Initialization ===\n");
    opencl_result = opencl_init(&env);
    if (opencl_result != 0) {
        (void)fprintf(stderr, "Failed to initialize OpenCL\n");
        return 1;
    }

    /* 5. Initialize cache directories for this algorithm */
    if (cache_init(algo->id, variants[variant_index]->variant_id) != 0) {
        (void)fprintf(stderr, "Warning: Failed to initialize cache directories for %s\n", algo->id);
    }

    /* 7. Run algorithm */
    (void)printf("\n=== Running %s (variant: %s) ===\n",
                 algo->name, variants[variant_index]->variant_id);
    run_algorithm(algo, variants[variant_index], &config, &env,
                  gpu_output_buffer, ref_output_buffer);

    /* Cleanup */
    opencl_cleanup(&env);
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
 * @param[in] provided_variant_index Variant index from command line, or -1 for interactive
 * @param[out] selected_algo Pointer to receive the selected algorithm
 * @param[out] variants Array to receive variant configurations
 * @param[out] variant_count Number of variants found
 * @param[out] selected_variant_index The selected variant index
 * @return 0 on success, -1 on error
 */
static int select_algorithm_and_variant(const Config* config, int provided_variant_index,
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

    /* Step 1: Display available algorithms */
    (void)printf("\n=== Available Algorithms ===\n");
    list_algorithms();
    (void)printf("\n");

    /* Step 2: Find selected algorithm based on config.op_id */
    algo = find_algorithm(config->op_id);
    if (algo == NULL) {
        (void)fprintf(stderr, "Error: Algorithm '%s' (from config) not found\n", config->op_id);
        (void)fprintf(stderr, "Please select from the available algorithms listed above.\n");
        return -1;
    }
    (void)printf("Selected algorithm from config: %s (ID: %s)\n", algo->name, algo->id);

    /* Step 3: Get kernel variants for selected algorithm */
    get_variants_result = get_op_variants(config, algo->id, variants, variant_count);
    if ((get_variants_result != 0) || (*variant_count == 0)) {
        (void)fprintf(stderr, "No kernel variants configured for %s\n", algo->name);
        return -1;
    }

    /* Step 4: Display available variants */
    (void)printf("=== Available Variants for %s ===\n", algo->name);
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
            if (!safe_strtol(input_buffer, &temp_index)) {
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
