/**
 * @file kernel_args.c
 * @brief Kernel argument handling for OpenCL kernels
 *
 * Provides functions to set kernel arguments based on configuration:
 * - Lookup OpParams fields by name (int, float, size_t)
 * - Set kernel arguments from KernelConfig descriptors
 * - Support for buffers, scalars, and packed structs
 *
 * MISRA C 2023 Compliance:
 * - Rule 17.7: All OpenCL API return values checked
 */

#include "kernel_args.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "op_interface.h"
#include "utils/config.h"

/* OpParams field lookup table for data-driven kernel argument mapping */
typedef struct {
    const char* name;
    size_t offset;
} OpParamsIntField;

static const OpParamsIntField kOpParamsIntFields[] = {
    {"src_width", offsetof(OpParams, src_width)},
    {"src_height", offsetof(OpParams, src_height)},
    {"src_stride", offsetof(OpParams, src_stride)},
    {"dst_width", offsetof(OpParams, dst_width)},
    {"dst_height", offsetof(OpParams, dst_height)},
    {"dst_stride", offsetof(OpParams, dst_stride)},
    {"kernel_variant", offsetof(OpParams, kernel_variant)},
    {NULL, 0} /* sentinel */
};

/**
 * @brief Lookup an int field in OpParams by name
 *
 * First checks built-in OpParams fields (src_width, dst_height, etc.),
 * then falls back to custom_scalars for algorithm-specific parameters.
 *
 * @param params The OpParams struct to lookup from
 * @param field_name The name of the field to find
 * @return Pointer to the int field, or NULL if not found
 */
static const int* OpParamsLookupInt(const OpParams* params, const char* field_name) {
    const OpParamsIntField* field;
    int i;

    if ((params == NULL) || (field_name == NULL)) {
        return NULL;
    }

    /* Check built-in fields first (fast path for common parameters) */
    for (field = kOpParamsIntFields; field->name != NULL; field++) {
        if (strcmp(field->name, field_name) == 0) {
            /* Calculate pointer to field using offset */
            return (const int*)((const char*)params + field->offset);
        }
    }

    /* Fallback to custom_scalars for algorithm-specific parameters */
    if (params->custom_scalars != NULL) {
        for (i = 0; i < params->custom_scalars->count; i++) {
            if ((params->custom_scalars->scalars[i].type == SCALAR_TYPE_INT) &&
                (strcmp(params->custom_scalars->scalars[i].name, field_name) == 0)) {
                return &params->custom_scalars->scalars[i].value.int_value;
            }
        }
    }

    return NULL;
}

/**
 * @brief Lookup a float field in custom_scalars by name
 *
 * Float fields are only supported via custom_scalars (no built-in float fields).
 *
 * @param params The OpParams struct to lookup from
 * @param field_name The name of the field to find
 * @return Pointer to the float field, or NULL if not found
 */
static const float* OpParamsLookupFloat(const OpParams* params, const char* field_name) {
    int i;

    if ((params == NULL) || (field_name == NULL)) {
        return NULL;
    }

    /* Float fields are only in custom_scalars */
    if (params->custom_scalars != NULL) {
        for (i = 0; i < params->custom_scalars->count; i++) {
            if ((params->custom_scalars->scalars[i].type == SCALAR_TYPE_FLOAT) &&
                (strcmp(params->custom_scalars->scalars[i].name, field_name) == 0)) {
                return &params->custom_scalars->scalars[i].value.float_value;
            }
        }
    }

    return NULL;
}

/**
 * @brief Lookup a size_t field in custom_scalars by name
 *
 * Size_t fields are only supported via custom_scalars (no built-in size_t fields).
 *
 * @param params The OpParams struct to lookup from
 * @param field_name The name of the field to find
 * @return Pointer to the size_t field, or NULL if not found
 */
static const size_t* OpParamsLookupSize(const OpParams* params, const char* field_name) {
    int i;

    if ((params == NULL) || (field_name == NULL)) {
        return NULL;
    }

    /* Size_t fields are only in custom_scalars */
    if (params->custom_scalars != NULL) {
        for (i = 0; i < params->custom_scalars->count; i++) {
            if ((params->custom_scalars->scalars[i].type == SCALAR_TYPE_SIZE) &&
                (strcmp(params->custom_scalars->scalars[i].name, field_name) == 0)) {
                return &params->custom_scalars->scalars[i].value.size_value;
            }
        }
    }

    return NULL;
}

/**
 * @brief Set a custom buffer kernel argument
 *
 * Finds and sets a custom buffer by name or numeric index.
 *
 * @param kernel OpenCL kernel
 * @param arg_idx Current argument index (will be incremented)
 * @param custom_buffers Available custom buffers
 * @param source_name Buffer name or numeric index string
 * @return 0 on success, -1 on error
 */
static int SetCustomBufferArg(cl_kernel kernel, cl_uint* arg_idx,
                              const CustomBuffers* custom_buffers, const char* source_name) {
    int buffer_idx = -1;
    int j;

    if (custom_buffers == NULL) {
        (void)fprintf(stderr,
                      "Error: Custom buffer '%s' requested but no custom buffers available\n",
                      source_name);
        return -1;
    }

    /* Check if source_name is a numeric index */
    if (source_name[0] >= '0' && source_name[0] <= '9') {
        /* Parse as integer index */
        buffer_idx = atoi(source_name);
        if (buffer_idx < 0 || buffer_idx >= custom_buffers->count) {
            (void)fprintf(stderr, "Error: Buffer index %d out of range (0-%d)\n",
                          buffer_idx, custom_buffers->count - 1);
            return -1;
        }
    } else {
        /* Look up by name */
        for (j = 0; j < custom_buffers->count; j++) {
            if (strcmp(custom_buffers->buffers[j].name, source_name) == 0) {
                buffer_idx = j;
                break;
            }
        }

        if (buffer_idx < 0) {
            (void)fprintf(stderr, "Error: Custom buffer '%s' not found\n", source_name);
            return -1;
        }
    }

    if (clSetKernelArg(kernel, (*arg_idx)++, sizeof(cl_mem),
                       &custom_buffers->buffers[buffer_idx].buffer) != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to set custom buffer '%s' at arg %d\n",
                      source_name, *arg_idx - 1);
        return -1;
    }

    return 0;
}

/**
 * @brief Set a size_t scalar kernel argument
 *
 * Handles both buffer.size format and custom scalar size_t values.
 *
 * @param kernel OpenCL kernel
 * @param arg_idx Current argument index (will be incremented)
 * @param params Operation parameters
 * @param custom_buffers Available custom buffers
 * @param source_name Source name (e.g., "buffer.size" or scalar name)
 * @return 0 on success, -1 on error
 */
static int SetSizeTArg(cl_kernel kernel, cl_uint* arg_idx, const OpParams* params,
                       const CustomBuffers* custom_buffers, const char* source_name) {
    const char* dot = strchr(source_name, '.');

    if (dot != NULL && strcmp(dot, ".size") == 0) {
        /* Buffer size format: "buffer_name.size" or "index.size" */
        char buffer_name[64];
        size_t name_len = (size_t)(dot - source_name);
        int buffer_idx = -1;
        int j;

        if (name_len >= sizeof(buffer_name)) {
            (void)fprintf(stderr, "Error: Buffer name/index too long in '%s'\n", source_name);
            return -1;
        }
        (void)strncpy(buffer_name, source_name, name_len);
        buffer_name[name_len] = '\0';

        if (custom_buffers == NULL) {
            (void)fprintf(stderr,
                          "Error: size_t arg '%s' requested but no custom buffers available\n",
                          source_name);
            return -1;
        }

        /* Check if buffer_name is a numeric index */
        if (buffer_name[0] >= '0' && buffer_name[0] <= '9') {
            buffer_idx = atoi(buffer_name);
            if (buffer_idx < 0 || buffer_idx >= custom_buffers->count) {
                (void)fprintf(stderr, "Error: Buffer index %d out of range (0-%d) in '%s'\n",
                              buffer_idx, custom_buffers->count - 1, source_name);
                return -1;
            }
        } else {
            /* Look up by name */
            for (j = 0; j < custom_buffers->count; j++) {
                if (strcmp(custom_buffers->buffers[j].name, buffer_name) == 0) {
                    buffer_idx = j;
                    break;
                }
            }

            if (buffer_idx < 0) {
                (void)fprintf(stderr, "Error: Buffer '%s' not found for size_t arg\n", buffer_name);
                return -1;
            }
        }

        /* Set the buffer size as unsigned long (OpenCL kernel convention) */
        unsigned long buffer_size = (unsigned long)custom_buffers->buffers[buffer_idx].size_bytes;
        if (clSetKernelArg(kernel, (*arg_idx)++, sizeof(unsigned long), &buffer_size) !=
            CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to set size_t arg for '%s' at arg %d\n",
                          buffer_name, *arg_idx - 1);
            return -1;
        }
    } else {
        /* Custom scalar size_t: lookup in custom_scalars */
        const size_t* value_ptr = OpParamsLookupSize(params, source_name);
        if (value_ptr == NULL) {
            (void)fprintf(stderr, "Error: Unknown size_t scalar source '%s'\n", source_name);
            return -1;
        }
        if (clSetKernelArg(kernel, (*arg_idx)++, sizeof(size_t), value_ptr) != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to set scalar size_t '%s' at arg %d\n",
                          source_name, *arg_idx - 1);
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Set a struct kernel argument
 *
 * Packs scalar fields into a struct buffer and sets as kernel argument.
 *
 * @param kernel OpenCL kernel
 * @param arg_idx Current argument index (will be incremented)
 * @param params Operation parameters (contains custom_scalars)
 * @param arg_desc Kernel argument descriptor with struct field info
 * @return 0 on success, -1 on error
 */
static int SetStructArg(cl_kernel kernel, cl_uint* arg_idx, const OpParams* params,
                        const KernelArgDescriptor* arg_desc) {
    unsigned char struct_buffer[256]; /* Max struct size */
    size_t struct_size = 0;
    int field_idx;

    if (params->custom_scalars == NULL) {
        (void)fprintf(stderr, "Error: Struct arg requires scalars section in config\n");
        return -1;
    }

    /* Pack each field into the struct buffer */
    for (field_idx = 0; field_idx < arg_desc->struct_field_count; field_idx++) {
        const char* field_name = arg_desc->struct_fields[field_idx];
        int scalar_idx;
        int found = 0;

        /* Find the scalar by name */
        for (scalar_idx = 0; scalar_idx < params->custom_scalars->count; scalar_idx++) {
            if (strcmp(params->custom_scalars->scalars[scalar_idx].name, field_name) == 0) {
                const ScalarValue* sv = &params->custom_scalars->scalars[scalar_idx];

                /* Pack based on type */
                switch (sv->type) {
                    case SCALAR_TYPE_INT:
                        if (struct_size + sizeof(int) > sizeof(struct_buffer)) {
                            (void)fprintf(stderr, "Error: Struct too large\n");
                            return -1;
                        }
                        (void)memcpy(struct_buffer + struct_size, &sv->value.int_value,
                                     sizeof(int));
                        struct_size += sizeof(int);
                        break;
                    case SCALAR_TYPE_FLOAT:
                        if (struct_size + sizeof(float) > sizeof(struct_buffer)) {
                            (void)fprintf(stderr, "Error: Struct too large\n");
                            return -1;
                        }
                        (void)memcpy(struct_buffer + struct_size, &sv->value.float_value,
                                     sizeof(float));
                        struct_size += sizeof(float);
                        break;
                    case SCALAR_TYPE_SIZE:
                        if (struct_size + sizeof(size_t) > sizeof(struct_buffer)) {
                            (void)fprintf(stderr, "Error: Struct too large\n");
                            return -1;
                        }
                        (void)memcpy(struct_buffer + struct_size, &sv->value.size_value,
                                     sizeof(size_t));
                        struct_size += sizeof(size_t);
                        break;
                    default:
                        (void)fprintf(stderr, "Error: Unknown scalar type for field '%s'\n",
                                      field_name);
                        return -1;
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            (void)fprintf(stderr, "Error: Struct field '%s' not found in scalars\n", field_name);
            return -1;
        }
    }

    /* Set the packed struct as kernel argument */
    if (clSetKernelArg(kernel, (*arg_idx)++, struct_size, struct_buffer) != CL_SUCCESS) {
        (void)fprintf(stderr, "Error: Failed to set struct arg at %d (size %zu)\n",
                      *arg_idx - 1, struct_size);
        return -1;
    }

    return 0;
}

int OpenclSetKernelArgs(cl_kernel kernel, cl_mem input_buf, cl_mem output_buf,
                        const OpParams* params, const KernelConfig* kernel_config) {
    cl_uint arg_idx = 0U;
    int i;
    const KernelArgDescriptor* arg_desc;
    CustomBuffers* custom_buffers;

    if ((kernel == NULL) || (params == NULL) || (kernel_config == NULL)) {
        return -1;
    }

    /* If no kernel_args configured, use default: input, output, width, height */
    if (kernel_config->kernel_arg_count == 0) {
        /* Set input and output as arg0 and arg1 */
        if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to set input buffer (arg 0)\n");
            return -1;
        }
        if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
            (void)fprintf(stderr, "Error: Failed to set output buffer (arg 1)\n");
            return -1;
        }
        if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) != CL_SUCCESS) {
            return -1;
        }
        if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) != CL_SUCCESS) {
            return -1;
        }
        return 0;
    }

    /* Use kernel_args configuration */
    custom_buffers = params->custom_buffers;

    (void)printf("\n=== Setting Kernel Arguments (variant: %d) ===\n", params->kernel_variant);
    (void)printf("Total kernel_args to set: %d\n", kernel_config->kernel_arg_count);

    for (i = 0; i < kernel_config->kernel_arg_count; i++) {
        arg_desc = &kernel_config->kernel_args[i];
        (void)printf("kernel_args[%d]: type=%d, source='%s'\n", i, arg_desc->arg_type,
                     arg_desc->source_name);

        switch (arg_desc->arg_type) {
            case KERNEL_ARG_TYPE_BUFFER_INPUT:
                /* Set input buffer */
                if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) != CL_SUCCESS) {
                    (void)fprintf(stderr, "Error: Failed to set input buffer at arg %d\n",
                                  arg_idx - 1);
                    return -1;
                }
                break;

            case KERNEL_ARG_TYPE_BUFFER_OUTPUT:
                /* Set output buffer */
                if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) != CL_SUCCESS) {
                    (void)fprintf(stderr, "Error: Failed to set output buffer at arg %d\n",
                                  arg_idx - 1);
                    return -1;
                }
                break;

            case KERNEL_ARG_TYPE_BUFFER_CUSTOM:
                if (SetCustomBufferArg(kernel, &arg_idx, custom_buffers, arg_desc->source_name) !=
                    0) {
                    return -1;
                }
                break;

            case KERNEL_ARG_TYPE_SCALAR_INT: {
                /* Lookup OpParams field by name using data-driven table */
                const int* value_ptr = OpParamsLookupInt(params, arg_desc->source_name);
                if (value_ptr == NULL) {
                    (void)fprintf(stderr, "Error: Unknown scalar source '%s'\n",
                                  arg_desc->source_name);
                    return -1;
                }
                if (clSetKernelArg(kernel, arg_idx++, sizeof(int), value_ptr) != CL_SUCCESS) {
                    (void)fprintf(stderr, "Error: Failed to set scalar int '%s' at arg %d\n",
                                  arg_desc->source_name, arg_idx - 1);
                    return -1;
                }
                break;
            }

            case KERNEL_ARG_TYPE_SCALAR_SIZE:
                if (SetSizeTArg(kernel, &arg_idx, params, custom_buffers, arg_desc->source_name) !=
                    0) {
                    return -1;
                }
                break;

            case KERNEL_ARG_TYPE_SCALAR_FLOAT: {
                /* Lookup float field in custom_scalars */
                const float* value_ptr = OpParamsLookupFloat(params, arg_desc->source_name);
                if (value_ptr == NULL) {
                    (void)fprintf(stderr, "Error: Unknown float scalar source '%s'\n",
                                  arg_desc->source_name);
                    return -1;
                }
                if (clSetKernelArg(kernel, arg_idx++, sizeof(float), value_ptr) != CL_SUCCESS) {
                    (void)fprintf(stderr, "Error: Failed to set scalar float '%s' at arg %d\n",
                                  arg_desc->source_name, arg_idx - 1);
                    return -1;
                }
                break;
            }

            case KERNEL_ARG_TYPE_STRUCT:
                if (SetStructArg(kernel, &arg_idx, params, arg_desc) != 0) {
                    return -1;
                }
                break;

            default:
                (void)fprintf(stderr, "Error: Unknown kernel arg type %d\n", arg_desc->arg_type);
                return -1;
        }
    }

    return 0;
}
