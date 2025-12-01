/**
 * @file algorithm_runner.c
 * @brief Algorithm execution pipeline implementation
 */

#include "algorithm_runner.h"
#include "utils/cache_manager.h"
#include "utils/image_io.h"
#include "utils/safe_ops.h"
#include "utils/verify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void run_algorithm(const Algorithm* algo, const KernelConfig* kernel_cfg,
                   const Config* config, OpenCLEnv* env,
                   unsigned char* gpu_output_buffer,
                   unsigned char* ref_output_buffer) {
    cl_int err;
    unsigned char* input;
    int img_size;
    cl_kernel kernel;
    cl_mem input_buf = NULL;
    cl_mem output_buf = NULL;
    CustomBuffers custom_buffers = {0};
    clock_t ref_start;
    clock_t ref_end;
    double ref_time;
    double gpu_time;
    float max_error;
    int passed;
    int write_result;
    size_t img_size_t;
    int run_result;
    int i;
    OpParams op_params = {0};  /* Common params reused throughout */

    if ((algo == NULL) || (kernel_cfg == NULL) || (config == NULL) || (env == NULL)) {
        (void)fprintf(stderr, "Error: NULL parameter in run_algorithm\n");
        return;
    }

    if ((gpu_output_buffer == NULL) || (ref_output_buffer == NULL)) {
        (void)fprintf(stderr, "Error: NULL output buffers in run_algorithm\n");
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
    if (img_size > MAX_IMAGE_SIZE) {
        (void)fprintf(stderr, "Image too large for static buffers\n");
        return;
    }

    /* Initialize common OpParams fields */
    op_params.src_width = config->src_width;
    op_params.src_height = config->src_height;
    op_params.dst_width = config->src_width;  /* Most algorithms keep same size */
    op_params.dst_height = config->src_height;
    op_params.border_mode = BORDER_CLAMP;

    /* Step 0: Load custom buffer data from files (needed by both C ref and GPU) */
    if (config->custom_buffer_count > 0) {
        (void)printf("\n=== Loading Custom Buffer Data ===\n");

        for (i = 0; i < config->custom_buffer_count; i++) {
            const CustomBufferConfig* buf_cfg = &config->custom_buffers[i];
            RuntimeBuffer* runtime_buf = &custom_buffers.buffers[i];

            /* Set buffer configuration metadata */
            runtime_buf->type = buf_cfg->type;
            runtime_buf->size_bytes = buf_cfg->size_bytes;

            /* File-backed buffer: load data from file into host memory */
            if (buf_cfg->source_file[0] != '\0') {
                unsigned char* temp_data;

                /* Use read_image to load into static buffer */
                temp_data = read_image(buf_cfg->source_file,
                                      (int)buf_cfg->size_bytes, 1);
                if (temp_data == NULL) {
                    (void)fprintf(stderr, "Failed to load %s\n", buf_cfg->source_file);
                    custom_buffers.count = i;  /* Track how many were loaded before failure */
                    goto cleanup_early;
                }

                /* Allocate dynamic memory and copy data (since read_image returns static buffer) */
                runtime_buf->host_data = (unsigned char*)malloc(buf_cfg->size_bytes);
                if (runtime_buf->host_data == NULL) {
                    (void)fprintf(stderr, "Failed to allocate memory for %s\n", buf_cfg->name);
                    custom_buffers.count = i;
                    goto cleanup_early;
                }
                (void)memcpy(runtime_buf->host_data, temp_data, buf_cfg->size_bytes);

                (void)printf("Loaded '%s' from %s (%zu bytes)\n",
                           buf_cfg->name, buf_cfg->source_file, buf_cfg->size_bytes);
            } else {
                runtime_buf->host_data = NULL;
            }

            custom_buffers.count++;
        }

        /* Make custom buffer data available to reference implementation */
        op_params.custom_buffers = &custom_buffers;
    }

    /* Step 1: Run C reference implementation */
    (void)printf("\n=== C Reference Implementation ===\n");
    ref_start = clock();
    op_params.input = input;
    op_params.output = ref_output_buffer;
    algo->reference_impl(&op_params);
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

    /* Step 4: Create STANDARD OpenCL buffers (input, output) */
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

    /* Step 4b: Create OpenCL buffers from already-loaded custom buffer data */
    if (config->custom_buffer_count > 0) {
        cl_mem_flags mem_flags;

        (void)printf("\n=== Creating Custom GPU Buffers ===\n");

        for (i = 0; i < config->custom_buffer_count; i++) {
            const CustomBufferConfig* buf_cfg = &config->custom_buffers[i];
            RuntimeBuffer* runtime_buf = &custom_buffers.buffers[i];

            /* Convert buffer type to OpenCL flags */
            if (buf_cfg->type == BUFFER_TYPE_READ_ONLY) {
                mem_flags = CL_MEM_READ_ONLY;
            } else if (buf_cfg->type == BUFFER_TYPE_WRITE_ONLY) {
                mem_flags = CL_MEM_WRITE_ONLY;
            } else {
                mem_flags = CL_MEM_READ_WRITE;
            }

            /* Create GPU buffer from already-loaded host data */
            if (runtime_buf->host_data != NULL) {
                /* Create buffer with host data (already loaded earlier) */
                runtime_buf->buffer = opencl_create_buffer(env->context,
                                                          mem_flags | CL_MEM_COPY_HOST_PTR,
                                                          buf_cfg->size_bytes,
                                                          runtime_buf->host_data,
                                                          buf_cfg->name);
                (void)printf("Created GPU buffer '%s' from host data (%zu bytes)\n",
                           buf_cfg->name, buf_cfg->size_bytes);
            }
            /* Empty buffer: allocate without initialization */
            else {
                runtime_buf->buffer = opencl_create_buffer(env->context,
                                                          mem_flags,
                                                          buf_cfg->size_bytes,
                                                          NULL,
                                                          buf_cfg->name);
                (void)printf("Created empty GPU buffer '%s' (%zu bytes)\n",
                           buf_cfg->name, buf_cfg->size_bytes);
            }

            if (runtime_buf->buffer == NULL) {
                (void)fprintf(stderr, "Failed to create GPU buffer '%s'\n", buf_cfg->name);
                goto cleanup;
            }
        }
    }

    /* Step 5: Run OpenCL kernel (algorithm handles argument setting) */
    (void)printf("\n=== Running OpenCL Kernel ===\n");

    /* Note: op_params.custom_buffers already set earlier if custom buffers exist */
    /* Set host type and kernel variant for this kernel */
    op_params.host_type = kernel_cfg->host_type;
    op_params.kernel_variant = kernel_cfg->kernel_variant;

    run_result = opencl_run_kernel(env, kernel, algo,
                                  input_buf, output_buf, &op_params,
                                  kernel_cfg->global_work_size,
                                  kernel_cfg->local_work_size,
                                  kernel_cfg->work_dim,
                                  kernel_cfg->host_type,
                                  &gpu_time);
    if (run_result != 0) {
        (void)fprintf(stderr, "Failed to run kernel\n");
        goto cleanup;
    }

    (void)printf("GPU kernel time: %.3f ms\n", gpu_time);

    /* Step 6: Read back results */
    err = clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0U,
                             img_size_t, gpu_output_buffer, 0U, NULL, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to read output buffer (error code: %d)\n", err);
        goto cleanup;
    }

    /* Step 7: Verify GPU results against C reference */
    op_params.gpu_output = gpu_output_buffer;
    op_params.ref_output = ref_output_buffer;
    passed = algo->verify_result(&op_params, &max_error);

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

cleanup:
    /* Cleanup custom buffers */
    for (i = 0; i < custom_buffers.count; i++) {
        /* Release OpenCL buffer */
        if (custom_buffers.buffers[i].buffer != NULL) {
            opencl_release_mem_object(custom_buffers.buffers[i].buffer,
                                     config->custom_buffers[i].name);
        }
        /* Free host data */
        if (custom_buffers.buffers[i].host_data != NULL) {
            free(custom_buffers.buffers[i].host_data);
            custom_buffers.buffers[i].host_data = NULL;
        }
    }

    /* Cleanup standard buffers - MISRA-C:2023 Rule 22.1: Proper resource management */
    opencl_release_mem_object(output_buf, "output buffer");
    opencl_release_mem_object(input_buf, "input buffer");
    opencl_release_kernel(kernel);

    /* NOTE: input points to static buffer from read_image(), don't free it */
    return;

cleanup_early:
    /* Early cleanup before OpenCL resources were created */
    /* Free any custom buffer host data that was loaded */
    for (i = 0; i < custom_buffers.count; i++) {
        if (custom_buffers.buffers[i].host_data != NULL) {
            free(custom_buffers.buffers[i].host_data);
            custom_buffers.buffers[i].host_data = NULL;
        }
    }
    /* NOTE: input points to static buffer from read_image(), don't free it */
}
