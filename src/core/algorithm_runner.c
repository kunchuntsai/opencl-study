/**
 * @file algorithm_runner.c
 * @brief Algorithm execution pipeline implementation
 */

/* Internal implementation - includes full type definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "op_registry.h"
#include "platform/cache_manager.h"
#include "platform/opencl_utils.h"
#include "utils/config.h"
#include "utils/image_io.h"
#include "utils/safe_ops.h"
#include "utils/verify.h"

/** Maximum image size (used for static buffer allocation) */
#define MAX_IMAGE_SIZE (4096 * 4096)

void RunAlgorithm(const Algorithm* algo, const KernelConfig* kernel_cfg, const Config* config,
                  OpenCLEnv* env, unsigned char* gpu_output_buffer,
                  unsigned char* ref_output_buffer) {
    cl_int err;
    unsigned char* input;
    int img_size;
    cl_kernel kernel;
    cl_mem input_buf = NULL;
    cl_mem output_buf = NULL;
    CustomBuffers custom_buffers = {0};
    CustomScalars custom_scalars = {0};
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
    OpParams op_params = {0}; /* Common params reused throughout */

    if ((algo == NULL) || (kernel_cfg == NULL) || (config == NULL) || (env == NULL)) {
        (void)fprintf(stderr, "Error: NULL parameter in RunAlgorithm\n");
        return;
    }

    if ((gpu_output_buffer == NULL) || (ref_output_buffer == NULL)) {
        (void)fprintf(stderr, "Error: NULL output buffers in RunAlgorithm\n");
        return;
    }

    /* Load input image from config/inputs.ini */
    if (config->input_image_count == 0) {
        (void)fprintf(stderr, "Error: No input images configured in config/inputs.ini\n");
        return;
    }

    /* Find the specified input image by input_image_id */
    {
        const InputImageConfig* img_cfg = NULL;
        int selected_index = 0;
        int i;

        /* If input_image_id is specified, find matching image */
        if (config->input_image_id[0] != '\0') {
            for (i = 0; i < config->input_image_count; i++) {
                /* Extract image_N from section name for comparison */
                char image_name[64];
                (void)snprintf(image_name, sizeof(image_name), "image_%d", i + 1);
                if (strcmp(config->input_image_id, image_name) == 0) {
                    img_cfg = &config->input_images[i];
                    selected_index = i + 1;
                    break;
                }
            }
            if (img_cfg == NULL) {
                (void)fprintf(stderr, "Error: Specified input_image_id '%s' not found\n",
                              config->input_image_id);
                (void)fprintf(stderr, "Available images: image_1 to image_%d\n",
                              config->input_image_count);
                return;
            }
        } else {
            /* Default to first image if not specified */
            img_cfg = &config->input_images[0];
            selected_index = 1;
        }

        (void)printf("\n=== Loading Input Images ===\n");
        (void)printf("Using input image %d of %d: %s (%dx%d)\n", selected_index,
                     config->input_image_count, img_cfg->input_path, img_cfg->src_width,
                     img_cfg->src_height);

        input = ReadImage(img_cfg->input_path, img_cfg->src_width, img_cfg->src_height);
        if (input == NULL) {
            (void)fprintf(stderr, "Failed to load input image: %s\n", img_cfg->input_path);
            return;
        }

        /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
        if (!SafeMulInt(img_cfg->src_width, img_cfg->src_height, &img_size)) {
            (void)fprintf(stderr, "Image size overflow\n");
            return;
        }

        /* Initialize common OpParams fields */
        op_params.src_width = img_cfg->src_width;
        op_params.src_height = img_cfg->src_height;
        op_params.src_stride = img_cfg->src_stride;
    }

    /* Resolve output image configuration from config/outputs.ini */
    if (config->output_image_count == 0) {
        (void)fprintf(stderr, "Error: No output images configured in config/outputs.ini\n");
        return;
    }

    /* Find the specified output image by output_image_id */
    {
        const OutputImageConfig* out_cfg = NULL;
        int selected_index = 0;
        int i;

        /* If output_image_id is specified, find matching output */
        if (config->output_image_id[0] != '\0') {
            for (i = 0; i < config->output_image_count; i++) {
                /* Extract output_N from section name for comparison */
                char output_name[64];
                (void)snprintf(output_name, sizeof(output_name), "output_%d", i + 1);
                if (strcmp(config->output_image_id, output_name) == 0) {
                    out_cfg = &config->output_images[i];
                    selected_index = i + 1;
                    break;
                }
            }
            if (out_cfg == NULL) {
                (void)fprintf(stderr, "Error: Specified output_image_id '%s' not found\n",
                              config->output_image_id);
                (void)fprintf(stderr, "Available outputs: output_1 to output_%d\n",
                              config->output_image_count);
                return;
            }
        } else {
            /* Default to first output if not specified */
            out_cfg = &config->output_images[0];
            selected_index = 1;
        }

        (void)printf("\n=== Output Configuration ===\n");
        (void)printf("Using output image %d of %d: %s (%dx%d)\n", selected_index,
                     config->output_image_count, out_cfg->output_path, out_cfg->dst_width,
                     out_cfg->dst_height);

        /* Set output parameters from output image config */
        op_params.dst_width = out_cfg->dst_width;
        op_params.dst_height = out_cfg->dst_height;
        op_params.dst_stride = out_cfg->dst_stride;
    }

    /* Check if image fits in static buffers */
    if (img_size > MAX_IMAGE_SIZE) {
        (void)fprintf(stderr, "Image too large for static buffers\n");
        return;
    }

    op_params.border_mode = BORDER_CLAMP;

    /* Step 0: Load custom buffer data from files (needed by both C ref and GPU)
     */
    if (config->custom_buffer_count > 0) {
        (void)printf("\n=== Loading Custom Buffer Data ===\n");

        for (i = 0; i < config->custom_buffer_count; i++) {
            const CustomBufferConfig* buf_cfg = &config->custom_buffers[i];
            RuntimeBuffer* runtime_buf = &custom_buffers.buffers[i];

            /* Set buffer configuration metadata */
            (void)strncpy(runtime_buf->name, buf_cfg->name, sizeof(runtime_buf->name) - 1);
            runtime_buf->name[sizeof(runtime_buf->name) - 1] = '\0';
            runtime_buf->type = buf_cfg->type;
            runtime_buf->size_bytes = buf_cfg->size_bytes;

            /* File-backed buffer: load data from file into host memory */
            if (buf_cfg->source_file[0] != '\0') {
                unsigned char* temp_data;

                /* Use ReadImage to load into static buffer */
                temp_data = ReadImage(buf_cfg->source_file, (int)buf_cfg->size_bytes, 1);
                if (temp_data == NULL) {
                    (void)fprintf(stderr, "Failed to load %s\n", buf_cfg->source_file);
                    custom_buffers.count = i; /* Track how many were loaded before failure */
                    goto cleanup_early;
                }

                /* Allocate dynamic memory and copy data (since ReadImage returns
                 * static buffer) */
                runtime_buf->host_data = (unsigned char*)malloc(buf_cfg->size_bytes);
                if (runtime_buf->host_data == NULL) {
                    (void)fprintf(stderr, "Failed to allocate memory for %s\n", buf_cfg->name);
                    custom_buffers.count = i;
                    goto cleanup_early;
                }
                (void)memcpy(runtime_buf->host_data, temp_data, buf_cfg->size_bytes);

                (void)printf("Loaded '%s' from %s (%zu bytes)\n", buf_cfg->name,
                             buf_cfg->source_file, buf_cfg->size_bytes);
            } else {
                runtime_buf->host_data = NULL;
            }

            custom_buffers.count++;
        }

        /* Make custom buffer data available to reference implementation */
        op_params.custom_buffers = &custom_buffers;
    }

    /* Step 0b: Populate custom scalars from config */
    if (config->scalar_arg_count > 0) {
        (void)printf("\n=== Loading Custom Scalars ===\n");

        for (i = 0; i < config->scalar_arg_count; i++) {
            const ScalarArgConfig* scalar_cfg = &config->scalar_args[i];
            ScalarValue* scalar_val = &custom_scalars.scalars[i];

            /* Copy scalar configuration to runtime structure */
            (void)strncpy(scalar_val->name, scalar_cfg->name, sizeof(scalar_val->name) - 1);
            scalar_val->name[sizeof(scalar_val->name) - 1] = '\0';
            scalar_val->type = scalar_cfg->type;

            /* Copy value based on type */
            switch (scalar_cfg->type) {
                case SCALAR_TYPE_INT:
                    scalar_val->value.int_value = scalar_cfg->value.int_value;
                    (void)printf("Scalar '%s': int = %d\n", scalar_cfg->name,
                                 scalar_cfg->value.int_value);
                    break;
                case SCALAR_TYPE_FLOAT:
                    scalar_val->value.float_value = scalar_cfg->value.float_value;
                    (void)printf("Scalar '%s': float = %f\n", scalar_cfg->name,
                                 (double)scalar_cfg->value.float_value);
                    break;
                case SCALAR_TYPE_SIZE:
                    scalar_val->value.size_value = scalar_cfg->value.size_value;
                    (void)printf("Scalar '%s': size_t = %zu\n", scalar_cfg->name,
                                 scalar_cfg->value.size_value);
                    break;
                default:
                    (void)fprintf(stderr, "Warning: Unknown scalar type for '%s'\n",
                                  scalar_cfg->name);
                    break;
            }

            custom_scalars.count++;
        }

        /* Make custom scalars available via OpParams */
        op_params.custom_scalars = &custom_scalars;
    }

    /* Step 1: Get golden/reference output (either from C ref or from file) */
    if (config->verification.golden_source == GOLDEN_SOURCE_FILE) {
        /* Load golden directly from file (skip c_ref execution) */
        int load_result;

        (void)printf("\n=== Loading Golden Sample from File ===\n");
        if (config->verification.golden_file[0] == '\0') {
            (void)fprintf(stderr, "Error: golden_source=file but golden_file not specified\n");
            goto cleanup_early;
        }

        load_result = CacheLoadGoldenFromFile(config->verification.golden_file, ref_output_buffer,
                                              (size_t)img_size);
        if (load_result != 0) {
            (void)fprintf(stderr, "Failed to load golden file: %s\n",
                          config->verification.golden_file);
            goto cleanup_early;
        }

        ref_time = 0.0; /* No c_ref execution time */
    } else {
        /* Default: Run C reference implementation to generate golden */
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
        if (CacheGoldenExists(algo->id, NULL) != 0) {
            /* Golden sample exists - verify c_ref against it */
            size_t golden_differences;
            int golden_result;

            (void)printf("Golden sample found, verifying c_ref output...\n");
            golden_result = CacheVerifyGolden(algo->id, NULL, ref_output_buffer, (size_t)img_size,
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
            save_result = CacheSaveGolden(algo->id, NULL, ref_output_buffer, (size_t)img_size);
            if (save_result == 0) {
                (void)printf("Golden sample created successfully\n");
            } else {
                (void)fprintf(stderr, "Failed to create golden sample\n");
            }
        }
    }

    /* Step 3: Build OpenCL kernel */
    (void)printf("\n=== Building OpenCL Kernel ===\n");
    kernel = OpenclBuildKernel(env, algo->id, kernel_cfg->kernel_file, kernel_cfg->kernel_function,
                               kernel_cfg->kernel_option, kernel_cfg->host_type,
                               kernel_cfg->embedded_headers, kernel_cfg->embedded_header_count);
    if (kernel == NULL) {
        (void)fprintf(stderr, "Failed to build kernel\n");
        return;
    }

    /* Step 4: Create STANDARD OpenCL buffers (input, output) */
    img_size_t = (size_t)img_size;
    input_buf = OpenclCreateBuffer(env->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   img_size_t, input, "input");
    if (input_buf == NULL) {
        OpenclReleaseKernel(kernel);
        return;
    }

    output_buf = OpenclCreateBuffer(env->context, CL_MEM_WRITE_ONLY, img_size_t, NULL, "output");
    if (output_buf == NULL) {
        OpenclReleaseMemObject(input_buf, "input buffer");
        OpenclReleaseKernel(kernel);
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
                runtime_buf->buffer =
                    OpenclCreateBuffer(env->context, mem_flags | CL_MEM_COPY_HOST_PTR,
                                       buf_cfg->size_bytes, runtime_buf->host_data, buf_cfg->name);
                (void)printf("Created GPU buffer '%s' from host data (%zu bytes)\n", buf_cfg->name,
                             buf_cfg->size_bytes);
            }
            /* Empty buffer: allocate without initialization */
            else {
                runtime_buf->buffer = OpenclCreateBuffer(env->context, mem_flags,
                                                         buf_cfg->size_bytes, NULL, buf_cfg->name);
                (void)printf("Created empty GPU buffer '%s' (%zu bytes)\n", buf_cfg->name,
                             buf_cfg->size_bytes);
            }

            if (runtime_buf->buffer == NULL) {
                (void)fprintf(stderr, "Failed to create GPU buffer '%s'\n", buf_cfg->name);
                goto cleanup;
            }
        }
    }

    /* Step 5: Run OpenCL kernel (algorithm handles argument setting) */
    (void)printf("\n=== Running OpenCL Kernel ===\n");

    /* Note: op_params.custom_buffers already set earlier if custom buffers exist
     */
    /* Set host type and kernel variant for this kernel */
    op_params.host_type = kernel_cfg->host_type;
    op_params.kernel_variant = kernel_cfg->kernel_variant;

    run_result =
        OpenclRunKernel(env, kernel, algo, input_buf, output_buf, &op_params,
                        kernel_cfg->global_work_size, kernel_cfg->local_work_size,
                        kernel_cfg->work_dim, kernel_cfg, kernel_cfg->host_type, &gpu_time);
    if (run_result != 0) {
        (void)fprintf(stderr, "Failed to run kernel\n");
        goto cleanup;
    }

    (void)printf("GPU kernel time: %.3f ms\n", gpu_time);

    /* Step 6: Read back results */
    err = clEnqueueReadBuffer(env->queue, output_buf, CL_TRUE, 0U, img_size_t, gpu_output_buffer,
                              0U, NULL, NULL);
    if (err != CL_SUCCESS) {
        (void)fprintf(stderr, "Failed to read output buffer (error code: %d)\n", err);
        goto cleanup;
    }

    /* Step 7: Verify GPU results against C reference using config-driven tolerance */
    op_params.gpu_output = gpu_output_buffer;
    op_params.ref_output = ref_output_buffer;
    passed = VerifyWithTolerance(gpu_output_buffer, ref_output_buffer, op_params.dst_width,
                                 op_params.dst_height, config->verification.tolerance,
                                 config->verification.error_rate_threshold, &max_error);

    /* Display results */
    (void)printf("\n=== Results ===\n");
    if (config->verification.golden_source == GOLDEN_SOURCE_FILE) {
        (void)printf("Golden source:    file (%s)\n", config->verification.golden_file);
    } else {
        (void)printf("C Reference time: %.3f ms\n", ref_time);
        (void)printf("Speedup:          %.2fx\n", ref_time / gpu_time);
    }
    (void)printf("OpenCL GPU time:  %.3f ms\n", gpu_time);
    (void)printf("Verification:     %s\n", (passed != 0) ? "PASSED" : "FAILED");
    (void)printf("Max error:        %.2f\n", (double)max_error);

    /* Save output to timestamped run directory */
    {
        char output_path[512];
        const char* run_dir = CacheGetRunDir();
        if (run_dir != NULL) {
            (void)snprintf(output_path, sizeof(output_path), "%s/out.bin", run_dir);
            write_result = WriteImage(output_path, gpu_output_buffer, op_params.src_width,
                                      op_params.src_height);
            if (write_result == 0) {
                (void)printf("Output saved to: %s\n", output_path);
            } else {
                (void)fprintf(stderr, "Failed to save output image\n");
            }
        }
    }

cleanup:
    /* Cleanup custom buffers */
    for (i = 0; i < custom_buffers.count; i++) {
        /* Release OpenCL buffer */
        if (custom_buffers.buffers[i].buffer != NULL) {
            OpenclReleaseMemObject(custom_buffers.buffers[i].buffer,
                                   config->custom_buffers[i].name);
        }
        /* Free host data */
        if (custom_buffers.buffers[i].host_data != NULL) {
            free(custom_buffers.buffers[i].host_data);
            custom_buffers.buffers[i].host_data = NULL;
        }
    }

    /* Cleanup standard buffers - MISRA-C:2023 Rule 22.1: Proper resource
     * management */
    OpenclReleaseMemObject(output_buf, "output buffer");
    OpenclReleaseMemObject(input_buf, "input buffer");
    OpenclReleaseKernel(kernel);

    /* NOTE: input points to static buffer from ReadImage(), don't free it */
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
    /* NOTE: input points to static buffer from ReadImage(), don't free it */
}
