#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils/opencl_utils.h"
#include "utils/config_parser.h"
#include "utils/image_io.h"
#include "utils/op_registry.h"
#include "utils/safe_ops.h"
#include "dilate/dilate3x3.h"
#include "gaussian/gaussian5x5.h"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_IMAGE_BUFFER_SIZE (4096 * 4096)

#define CONFIG_FILE "config/config.ini"

static unsigned char gpu_output_buffer[MAX_IMAGE_BUFFER_SIZE];
static unsigned char ref_output_buffer[MAX_IMAGE_BUFFER_SIZE];

/* Forward declarations */
static void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                         const Config* config, OpenCLEnv* env);
static void register_all_algorithms(void);

int main(int argc, char** argv) {
    Config config;
    OpenCLEnv env;
    int algo_index;
    int variant_index;
    Algorithm* algo;
    KernelConfig* variants[MAX_KERNEL_CONFIGS];
    int variant_count;
    int parse_result;
    int opencl_result;
    int get_variants_result;
    long temp_index;

    /* Check command line arguments */
    if (argc != 3) {
        (void)fprintf(stderr, "Usage: %s <algorithm_index> <variant_index>\n", argv[0]);
        (void)fprintf(stderr, "  algorithm_index: 0=dilate3x3, 1=gaussian5x5\n");
        (void)fprintf(stderr, "  variant_index:   0=v0, 1=v1, ...\n");
        return 1;
    }

    /* Parse command line arguments - MISRA-C:2023 Rule 21.8: Avoid atoi() */
    if (!safe_strtol(argv[1], &temp_index)) {
        (void)fprintf(stderr, "Invalid algorithm index: %s\n", argv[1]);
        return 1;
    }
    algo_index = (int)temp_index;

    if (!safe_strtol(argv[2], &temp_index)) {
        (void)fprintf(stderr, "Invalid variant index: %s\n", argv[2]);
        return 1;
    }
    variant_index = (int)temp_index;

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

    /* 4. Get selected algorithm */
    algo = get_algorithm_by_index(algo_index);
    if (algo == NULL) {
        (void)fprintf(stderr, "Invalid algorithm index: %d\n", algo_index);
        opencl_cleanup(&env);
        return 1;
    }

    /* 5. Get kernel variants for selected algorithm */
    get_variants_result = get_op_variants(&config, algo->id, variants, &variant_count);
    if ((get_variants_result != 0) || (variant_count == 0)) {
        (void)fprintf(stderr, "No kernel variants configured for %s\n", algo->name);
        opencl_cleanup(&env);
        return 1;
    }

    if ((variant_index < 0) || (variant_index >= variant_count)) {
        (void)fprintf(stderr, "Invalid variant index: %d (available: 0-%d)\n",
                      variant_index, variant_count - 1);
        opencl_cleanup(&env);
        return 1;
    }

    /* 6. Run algorithm */
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
    input = read_image(config->input_image, config->width, config->height);
    if (input == NULL) {
        (void)fprintf(stderr, "Failed to load input image\n");
        return;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!safe_mul_int(config->width, config->height, &img_size)) {
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
    algo->reference_impl(input, ref_output_buffer, config->width, config->height);
    ref_end = clock();
    ref_time = (double)(ref_end - ref_start) / (double)CLOCKS_PER_SEC * 1000.0;
    (void)printf("Reference time: %.3f ms\n", ref_time);

    /* Step 2: Build OpenCL kernel */
    (void)printf("\n=== Building OpenCL Kernel ===\n");
    kernel = opencl_build_kernel(env, kernel_cfg->kernel_file,
                                  kernel_cfg->kernel_function);
    if (kernel == NULL) {
        (void)fprintf(stderr, "Failed to build kernel\n");
        return;
    }

    /* Step 3: Create OpenCL buffers */
    img_size_t = (size_t)img_size;
    input_buf = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               img_size_t, input, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to create input buffer (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return value */
        err = clReleaseKernel(kernel);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
        }
        return;
    }

    output_buf = clCreateBuffer(env->context, CL_MEM_WRITE_ONLY,
                                img_size_t, NULL, &err);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to create output buffer (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return values */
        err = clReleaseMemObject(input_buf);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release input buffer (error: %d)\n", err);
        }
        err = clReleaseKernel(kernel);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
        }
        return;
    }

    /* Step 4: Run OpenCL kernel */
    (void)printf("\n=== Running OpenCL Kernel ===\n");
    run_result = opencl_run_kernel(env, kernel, input_buf, output_buf,
                                   config->width, config->height,
                                   kernel_cfg->global_work_size,
                                   kernel_cfg->local_work_size,
                                   kernel_cfg->work_dim,
                                   &gpu_time);
    if (run_result != 0) {
        (void)fprintf(stderr, "Failed to run kernel\n");
        /* MISRA-C:2023 Rule 17.7: Check return values */
        err = clReleaseMemObject(output_buf);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release output buffer (error: %d)\n", err);
        }
        err = clReleaseMemObject(input_buf);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release input buffer (error: %d)\n", err);
        }
        err = clReleaseKernel(kernel);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
        }
        return;
    }

    (void)printf("GPU kernel time: %.3f ms\n", gpu_time);

    /* Step 5: Read back results */
    err = clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0U,
                             img_size_t, gpu_output_buffer, 0U, NULL, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to read output buffer (error code: %d)\n", err);
        /* MISRA-C:2023 Rule 17.7: Check return values */
        err = clReleaseMemObject(output_buf);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release output buffer (error: %d)\n", err);
        }
        err = clReleaseMemObject(input_buf);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release input buffer (error: %d)\n", err);
        }
        err = clReleaseKernel(kernel);
        if (err != CL_SUCCESS) {
            (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
        }
        return;
    }

    /* Step 6: Verify results */
    passed = algo->verify_result(gpu_output_buffer, ref_output_buffer,
                                 config->width, config->height, &max_error);

    /* Display results */
    (void)printf("\n=== Results ===\n");
    (void)printf("C Reference time: %.3f ms\n", ref_time);
    (void)printf("OpenCL GPU time:  %.3f ms\n", gpu_time);
    (void)printf("Speedup:          %.2fx\n", ref_time / gpu_time);
    (void)printf("Verification:     %s\n", (passed != 0) ? "PASSED" : "FAILED");
    (void)printf("Max error:        %.2f\n", (double)max_error);

    /* Save output */
    write_result = write_image(config->output_image, gpu_output_buffer,
                              config->width, config->height);
    if (write_result == 0) {
        (void)printf("Output saved to: %s\n", config->output_image);
    } else {
        (void)fprintf(stderr, "Failed to save output image\n");
    }

    /* Cleanup - MISRA-C:2023 Rule 22.1: Proper resource management */
    /* MISRA-C:2023 Rule 17.7: Check return values */
    err = clReleaseMemObject(output_buf);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to release output buffer (error: %d)\n", err);
    }

    err = clReleaseMemObject(input_buf);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to release input buffer (error: %d)\n", err);
    }

    err = clReleaseKernel(kernel);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Warning: Failed to release kernel (error: %d)\n", err);
    }
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
