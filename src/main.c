#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils/opencl_utils.h"
#include "utils/cache_manager.h"
#include "utils/config_parser.h"
#include "utils/image_io.h"
#include "utils/op_registry.h"
#include "utils/safe_ops.h"
#include "dilate/dilate3x3.h"
#include "gaussian/gaussian5x5.h"

#define CONFIG_FILE "config/config.ini"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_IMAGE_BUFFER_SIZE (4096 * 4096)

static unsigned char gpu_output_buffer[MAX_IMAGE_BUFFER_SIZE];
static unsigned char ref_output_buffer[MAX_IMAGE_BUFFER_SIZE];

/* Forward declarations */
static void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                         const Config* config, OpenCLEnv* env);
static void register_all_algorithms(void);

int main(int argc, char** argv) {
    Config config;
    OpenCLEnv env;
    int variant_index;
    Algorithm* algo;
    KernelConfig* variants[MAX_KERNEL_CONFIGS];
    int variant_count;
    int parse_result;
    int opencl_result;
    int get_variants_result;
    long temp_index;

    /* Check command line arguments */
    if ((argc != 1) && (argc != 2)) {
        (void)fprintf(stderr, "Usage: %s [variant_index]\n", argv[0]);
        (void)fprintf(stderr, "  variant_index:   0=v0, 1=v1, ... (default: 0)\n");
        (void)fprintf(stderr, "  Note: Algorithm is selected from config.ini (op_id)\n");
        return 1;
    }

    /* Parse command line arguments - MISRA-C:2023 Rule 21.8: Avoid atoi() */
    if (argc == 2) {
        if (!safe_strtol(argv[1], &temp_index)) {
            (void)fprintf(stderr, "Invalid variant index: %s\n", argv[1]);
            return 1;
        }
        variant_index = (int)temp_index;
    } else {
        /* Variant will be selected interactively */
        variant_index = -1;
    }

    /* 1. Parse configuration */
    parse_result = parse_config(CONFIG_FILE, &config);
    if (parse_result != 0) {
        (void)fprintf(stderr, "Failed to parse %s\n", CONFIG_FILE);
        return 1;
    }

    /* 2. Initialize OpenCL */
    (void)printf("=== OpenCL Initialization ===\n");
    opencl_result = opencl_init(&env);
    if (opencl_result != 0) {
        (void)fprintf(stderr, "Failed to initialize OpenCL\n");
        return 1;
    }

    /* 3. Register all algorithms */
    register_all_algorithms();

    /* Display available algorithms */
    (void)printf("\n=== Available Algorithms ===\n");
    list_algorithms();
    (void)printf("\n");

    /* 4. Get selected algorithm based on config.op_id */
    algo = find_algorithm(config.op_id);
    if (algo == NULL) {
        (void)fprintf(stderr, "Error: Algorithm '%s' (from config) not found\n", config.op_id);
        (void)fprintf(stderr, "Please select from the available algorithms listed above.\n");
        opencl_cleanup(&env);
        return 1;
    }
    (void)printf("Selected algorithm from config: %s (ID: %s)\n", algo->name, algo->id);

    /* 5. Initialize cache directories for this algorithm */
    if (cache_init(algo->id) != 0) {
        (void)fprintf(stderr, "Warning: Failed to initialize cache directories for %s\n", algo->id);
    }

    /* 6. Get kernel variants for selected algorithm */
    get_variants_result = get_op_variants(&config, algo->id, variants, &variant_count);
    if ((get_variants_result != 0) || (variant_count == 0)) {
        (void)fprintf(stderr, "No kernel variants configured for %s\n", algo->name);
        opencl_cleanup(&env);
        return 1;
    }

    /* Display available variants */
    (void)printf("=== Available Variants for %s ===\n", algo->name);
    {
        int i;
        for (i = 0; i < variant_count; i++) {
            (void)printf("  %d - %s\n", i, variants[i]->variant_id);
        }
        (void)printf("\n");
    }

    /* If variant_index not provided via command line, prompt user */
    if (argc == 1) {
        char input_buffer[32];
        char* newline_pos;
        (void)printf("Select variant (0-%d, default: 0): ", variant_count - 1);
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            /* Remove trailing newline if present */
            newline_pos = strchr(input_buffer, '\n');
            if (newline_pos != NULL) {
                *newline_pos = '\0';
            }

            /* If empty input, use default variant 0 */
            if (input_buffer[0] == '\0') {
                variant_index = 0;
            } else {
                if (!safe_strtol(input_buffer, &temp_index)) {
                    (void)fprintf(stderr, "Invalid variant selection\n");
                    opencl_cleanup(&env);
                    return 1;
                }
                variant_index = (int)temp_index;
            }
        } else {
            (void)fprintf(stderr, "Failed to read input\n");
            opencl_cleanup(&env);
            return 1;
        }
    }

    if ((variant_index < 0) || (variant_index >= variant_count)) {
        (void)fprintf(stderr, "Error: Invalid variant index: %d (available: 0-%d)\n",
                      variant_index, variant_count - 1);
        opencl_cleanup(&env);
        return 1;
    }

    /* 7. Run algorithm */
    (void)printf("\n=== Running %s (variant: %s) ===\n",
                 algo->name, variants[variant_index]->variant_id);
    run_algorithm(algo, variants[variant_index], &config, &env);

    /* Cleanup */
    opencl_cleanup(&env);
    return 0;
}

static void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                         const Config* config, OpenCLEnv* env) {
    cl_int err;
    unsigned char* input;
    int img_size;
    cl_kernel kernel;
    cl_mem input_buf = NULL;
    cl_mem output_buf = NULL;
    clock_t ref_start;
    clock_t ref_end;
    double ref_time;
    double gpu_time;
    float max_error;
    int passed;
    int write_result;
    size_t img_size_t;
    int run_result;

    if ((algo == NULL) || (kernel_cfg == NULL) || (config == NULL) || (env == NULL)) {
        (void)fprintf(stderr, "Error: NULL parameter in run_algorithm\n");
        return;
    }

    /* Load input image */
    input = read_image(config->input_image, config->src_width, config->src_height);
    if (input == NULL) {
        (void)fprintf(stderr, "Failed to load input image\n");
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!safe_mul_int(config->src_width, config->src_height, &img_size)) {
        (void)fprintf(stderr, "Image size overflow\n");
        return;
    }

    /* Check if image fits in static buffers */
    if (img_size > MAX_IMAGE_BUFFER_SIZE) {
        (void)fprintf(stderr, "Image too large for static buffers\n");
        return;
    }

    /* Step 1: Run C reference implementation */
    (void)printf("\n=== C Reference Implementation ===\n");
    ref_start = clock();
    algo->reference_impl(input, ref_output_buffer, config->src_width, config->src_height);
    ref_end = clock();
    ref_time = (double)(ref_end - ref_start) / (double)CLOCKS_PER_SEC * 1000.0;
    (void)printf("Reference time: %.3f ms\n", ref_time);

    /* Step 2: Golden sample verification (c_ref output only) */
    (void)printf("\n=== Golden Sample Verification ===\n");
    if (cache_golden_exists(algo->id, NULL) != 0) {
        /* Golden sample exists - verify c_ref against it */
        size_t golden_differences;
        int golden_result;

        (void)printf("Golden sample found, verifying c_ref output...\n");
        golden_result = cache_verify_golden(algo->id, NULL,
                                            ref_output_buffer, (size_t)img_size,
                                            &golden_differences);
        if (golden_result < 0) {
            (void)fprintf(stderr, "Golden verification failed\n");
        } else if (golden_result == 0) {
            (void)fprintf(stderr, "Warning: C reference output differs from golden sample\n");
        }
    } else {
        /* No golden sample - create it from C reference output */
        int save_result;

        (void)printf("No golden sample found, creating from C reference output...\n");
        save_result = cache_save_golden(algo->id, NULL,
                                        ref_output_buffer, (size_t)img_size);
        if (save_result == 0) {
            (void)printf("Golden sample created successfully\n");
        } else {
            (void)fprintf(stderr, "Failed to create golden sample\n");
        }
    }

    /* Step 3: Build OpenCL kernel */
    (void)printf("\n=== Building OpenCL Kernel ===\n");
    kernel = opencl_build_kernel(env, algo->id, kernel_cfg->kernel_file,
                                  kernel_cfg->kernel_function);
    if (kernel == NULL) {
        (void)fprintf(stderr, "Failed to build kernel\n");
        return;
    }

    /* Step 4: Create OpenCL buffers */
    img_size_t = (size_t)img_size;
    input_buf = opencl_create_buffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                     img_size_t, input, "input");
    if (input_buf == NULL) {
        opencl_release_kernel(kernel);
        return;
    }

    output_buf = opencl_create_buffer(env->context, CL_MEM_WRITE_ONLY,
                                      img_size_t, NULL, "output");
    if (output_buf == NULL) {
        opencl_release_mem_object(input_buf, "input buffer");
        opencl_release_kernel(kernel);
        return;
    }

    /* Step 5: Run OpenCL kernel */
    (void)printf("\n=== Running OpenCL Kernel ===\n");
    run_result = opencl_run_kernel(env, kernel, input_buf, output_buf,
                                   config->src_width, config->src_height,
                                   kernel_cfg->global_work_size,
                                   kernel_cfg->local_work_size,
                                   kernel_cfg->work_dim,
                                   &gpu_time);
    if (run_result != 0) {
        (void)fprintf(stderr, "Failed to run kernel\n");
        opencl_release_mem_object(output_buf, "output buffer");
        opencl_release_mem_object(input_buf, "input buffer");
        opencl_release_kernel(kernel);
        return;
    }

    (void)printf("GPU kernel time: %.3f ms\n", gpu_time);

    /* Step 6: Read back results */
    err = clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0U,
                             img_size_t, gpu_output_buffer, 0U, NULL, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to read output buffer (error code: %d)\n", err);
        opencl_release_mem_object(output_buf, "output buffer");
        opencl_release_mem_object(input_buf, "input buffer");
        opencl_release_kernel(kernel);
        return;
    }

    /* Step 7: Verify GPU results against C reference */
    passed = algo->verify_result(gpu_output_buffer, ref_output_buffer,
                                 config->src_width, config->src_height, &max_error);

    /* Display results */
    (void)printf("\n=== Results ===\n");
    (void)printf("C Reference time: %.3f ms\n", ref_time);
    (void)printf("OpenCL GPU time:  %.3f ms\n", gpu_time);
    (void)printf("Speedup:          %.2fx\n", ref_time / gpu_time);
    (void)printf("Verification:     %s\n", (passed != 0) ? "PASSED" : "FAILED");
    (void)printf("Max error:        %.2f\n", (double)max_error);

    /* Save output */
    write_result = write_image(config->output_image, gpu_output_buffer,
                              config->src_width, config->src_height);
    if (write_result == 0) {
        (void)printf("Output saved to: %s\n", config->output_image);
    } else {
        (void)fprintf(stderr, "Failed to save output image\n");
    }

    /* Cleanup - MISRA-C:2023 Rule 22.1: Proper resource management */
    opencl_release_mem_object(output_buf, "output buffer");
    opencl_release_mem_object(input_buf, "input buffer");
    opencl_release_kernel(kernel);
}

static void register_all_algorithms(void) {
    /* Array of all available algorithms */
    Algorithm* const algorithms[] = {
        &dilate3x3_algorithm,
        &gaussian5x5_algorithm,
        NULL  /* Sentinel value to mark end of array */
    };

    /* Register each algorithm in the array */
    int i = 0;
    while (algorithms[i] != NULL) {
        register_algorithm(algorithms[i]);
        i++;
    }
}
