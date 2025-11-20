#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils/opencl_utils.h"
#include "utils/config_parser.h"
#include "utils/image_io.h"
#include "utils/op_registry.h"
#include "dilate/dilate3x3.h"
#include "gaussian/gaussian5x5.h"

void run_algorithm(Algorithm* algo, KernelConfig* kernel_cfg,
                   Config* config, OpenCLEnv* env);

int main(int argc, char** argv) {
    Config config;
    OpenCLEnv env;

    // 1. Parse configuration
    if (parse_config("config/config.ini", &config) != 0) {
        fprintf(stderr, "Failed to parse config/config.ini\n");
        return 1;
    }

    // 2. Initialize OpenCL
    if (opencl_init(&env) != 0) {
        fprintf(stderr, "Failed to initialize OpenCL\n");
        return 1;
    }

    // 3. Register algorithms
    register_algorithm(&dilate3x3_algorithm);
    register_algorithm(&gaussian5x5_algorithm);

    // 4. Main menu loop
    while (1) {
        printf("\n=== OpenCL Image Processing Framework ===\n");
        list_algorithms();
        printf("0. Exit\n");
        printf("Select algorithm: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input\n");
            continue;
        }

        if (choice == 0) break;

        Algorithm* algo = get_algorithm_by_index(choice - 1);
        if (!algo) {
            printf("Invalid selection\n");
            continue;
        }

        // 5. Show kernel variants for selected algorithm
        KernelConfig* variants;
        int variant_count;
        get_op_variants(&config, algo->id, &variants, &variant_count);

        if (variant_count == 0) {
            printf("No kernel variants configured for %s\n", algo->name);
            continue;
        }

        printf("\nAvailable variants for %s:\n", algo->name);
        for (int i = 0; i < variant_count; i++) {
            KernelConfig* kc = &variants[i];
            printf("%d. %s (%s - %s)\n", i + 1, kc->variant_id,
                   kc->kernel_file, kc->kernel_function);
        }

        printf("Select variant: ");
        int variant_choice;
        if (scanf("%d", &variant_choice) != 1) {
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input\n");
            continue;
        }

        if (variant_choice < 1 || variant_choice > variant_count) {
            printf("Invalid variant\n");
            continue;
        }

        KernelConfig* selected = &variants[variant_choice - 1];

        // 6. Run algorithm
        run_algorithm(algo, selected, &config, &env);
    }

    // Cleanup
    opencl_cleanup(&env);
    printf("Goodbye!\n");
    return 0;
}

void run_algorithm(Algorithm* algo, KernelConfig* kernel_cfg,
                   Config* config, OpenCLEnv* env) {
    cl_int err;

    // Load input image
    unsigned char* input = read_image(config->input_image,
                                     config->width, config->height);
    if (!input) {
        fprintf(stderr, "Failed to load input image\n");
        return;
    }

    int img_size = config->width * config->height;

    // Allocate output buffers
    unsigned char* gpu_output = (unsigned char*)malloc(img_size);
    unsigned char* ref_output = (unsigned char*)malloc(img_size);

    if (!gpu_output || !ref_output) {
        fprintf(stderr, "Failed to allocate output buffers\n");
        free(input);
        free(gpu_output);
        free(ref_output);
        return;
    }

    // Run C reference
    printf("\nRunning C reference implementation...\n");
    clock_t ref_start = clock();
    algo->reference_impl(input, ref_output, config->width, config->height);
    clock_t ref_end = clock();
    double ref_time = (double)(ref_end - ref_start) / CLOCKS_PER_SEC * 1000;

    // Run OpenCL kernel
    printf("Running OpenCL kernel: %s...\n", kernel_cfg->kernel_function);
    cl_kernel kernel = opencl_load_kernel(env, kernel_cfg->kernel_file,
                                         kernel_cfg->kernel_function);
    if (!kernel) {
        fprintf(stderr, "Failed to load kernel\n");
        free(input);
        free(gpu_output);
        free(ref_output);
        return;
    }

    // Create buffers
    cl_mem input_buf = clCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      img_size, input, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create input buffer (error code: %d)\n", err);
        clReleaseKernel(kernel);
        free(input);
        free(gpu_output);
        free(ref_output);
        return;
    }

    cl_mem output_buf = clCreateBuffer(env->context, CL_MEM_WRITE_ONLY,
                                       img_size, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create output buffer (error code: %d)\n", err);
        clReleaseMemObject(input_buf);
        clReleaseKernel(kernel);
        free(input);
        free(gpu_output);
        free(ref_output);
        return;
    }

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buf);
    clSetKernelArg(kernel, 2, sizeof(int), &config->width);
    clSetKernelArg(kernel, 3, sizeof(int), &config->height);

    // Execute kernel
    cl_event event;
    err = clEnqueueNDRangeKernel(env->queue, kernel, kernel_cfg->work_dim,
                          NULL, kernel_cfg->global_work_size,
                          kernel_cfg->local_work_size[0] == 0 ? NULL : kernel_cfg->local_work_size,
                          0, NULL, &event);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to enqueue kernel (error code: %d)\n", err);
        clReleaseMemObject(input_buf);
        clReleaseMemObject(output_buf);
        clReleaseKernel(kernel);
        free(input);
        free(gpu_output);
        free(ref_output);
        return;
    }

    clFinish(env->queue);

    // Get execution time
    cl_ulong time_start, time_end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                           sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                           sizeof(time_end), &time_end, NULL);
    double gpu_time = (time_end - time_start) / 1000000.0; // Convert to ms

    // Read back results
    clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0,
                       img_size, gpu_output, 0, NULL, NULL);

    // Verify results
    float max_error;
    int passed = algo->verify_result(gpu_output, ref_output,
                                    config->width, config->height, &max_error);

    // Display results
    printf("\n=== Results ===\n");
    printf("C Reference time: %.3f ms\n", ref_time);
    printf("OpenCL GPU time:  %.3f ms\n", gpu_time);
    printf("Speedup:          %.2fx\n", ref_time / gpu_time);
    printf("Verification:     %s\n", passed ? "PASSED" : "FAILED");
    printf("Max error:        %.2f\n", max_error);

    // Save output
    write_image(config->output_image, gpu_output, config->width, config->height);
    printf("Output saved to: %s\n", config->output_image);

    // Cleanup
    free(input);
    free(gpu_output);
    free(ref_output);
    clReleaseMemObject(input_buf);
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseEvent(event);
}
