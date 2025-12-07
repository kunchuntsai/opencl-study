/* POSIX feature test macro for strtok_r */
#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/safe_ops.h"

/* Maximum line length for config file */
#define MAX_LINE_LENGTH 512
#define MAX_SECTION_LENGTH 64
#define MAX_BUFFER_LENGTH 256

/* Helper function to Trim whitespace */
static char* Trim(char* str) {
    char* end;

    /* Trim leading space */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while ((end > str) && isspace((unsigned char)*end)) {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    return str;
}

/* Parse kernel section name to extract variant_id */
/* Format: "kernel.<variant_id>" */
static int ParseKernelSection(const char* section, char* variant_id, size_t variant_id_size) {
    const char* first_dot;
    size_t variant_len;

    if ((section == NULL) || (variant_id == NULL)) {
        return -1;
    }

    /* Skip "kernel." prefix */
    if (strncmp(section, "kernel.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7; /* Points to start of variant_id */

    /* Extract variant_id */
    variant_len = strlen(first_dot);
    if ((variant_len == 0U) || (variant_len >= variant_id_size)) {
        return -1; /* variant_id empty or too long */
    }
    (void)strncpy(variant_id, first_dot, variant_id_size - 1U);
    variant_id[variant_id_size - 1U] = '\0';

    return 0;
}

/* Extract numeric variant number from variant_id */
/* Format: "v0" -> 0, "v1" -> 1, "v2" -> 2, etc. */
/* Returns -1 on error, variant number on success */
static int ExtractVariantNumber(const char* variant_id) {
    long temp_long;
    char* endptr;

    if (variant_id == NULL) {
        return -1;
    }

    /* Check if variant_id starts with 'v' */
    if (variant_id[0] != 'v') {
        return -1;
    }

    /* Skip 'v' prefix and parse the number */
    temp_long = strtol(variant_id + 1, &endptr, 10);

    /* Validate conversion */
    if ((endptr == (variant_id + 1)) || (*endptr != '\0') || (temp_long < 0) || (temp_long > 99)) {
        return -1; /* Invalid format or number out of range */
    }

    return (int)temp_long;
}

/* Parse buffer section name to extract buffer name */
/* Format: "buffer.<name>" */
static int ParseBufferSection(const char* section, char* name, size_t name_size) {
    const char* first_dot;
    size_t name_len;

    if ((section == NULL) || (name == NULL)) {
        return -1;
    }

    /* Skip "buffer." prefix */
    if (strncmp(section, "buffer.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7; /* Points to start of buffer name */

    /* Extract buffer name */
    name_len = strlen(first_dot);
    if ((name_len == 0U) || (name_len >= name_size)) {
        return -1; /* name empty or too long */
    }
    (void)strncpy(name, first_dot, name_size - 1U);
    name[name_size - 1U] = '\0';

    return 0;
}

/* Parse scalar section name to extract scalar name */
/* Format: "scalar.<name>" */
static int ParseScalarSection(const char* section, char* name, size_t name_size) {
    const char* first_dot;
    size_t name_len;

    if ((section == NULL) || (name == NULL)) {
        return -1;
    }

    /* Skip "scalar." prefix */
    if (strncmp(section, "scalar.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7; /* Points to start of scalar name */

    /* Extract scalar name */
    name_len = strlen(first_dot);
    if ((name_len == 0U) || (name_len >= name_size)) {
        return -1; /* name empty or too long */
    }
    (void)strncpy(name, first_dot, name_size - 1U);
    name[name_size - 1U] = '\0';

    return 0;
}

/* Parse data type from string */
static DataType ParseDataType(const char* str) {
    if (str == NULL) {
        return DATA_TYPE_NONE;
    }

    if (strcmp(str, "float") == 0) {
        return DATA_TYPE_FLOAT;
    } else if (strcmp(str, "uchar") == 0) {
        return DATA_TYPE_UCHAR;
    } else if (strcmp(str, "int") == 0) {
        return DATA_TYPE_INT;
    } else if (strcmp(str, "short") == 0) {
        return DATA_TYPE_SHORT;
    }

    return DATA_TYPE_NONE;
}

/* Get size in bytes for a data type */
static size_t GetDataTypeSize(DataType type) {
    switch (type) {
        case DATA_TYPE_FLOAT:
            return sizeof(float);
        case DATA_TYPE_UCHAR:
            return sizeof(unsigned char);
        case DATA_TYPE_INT:
            return sizeof(int);
        case DATA_TYPE_SHORT:
            return sizeof(short);
        default:
            return 0;
    }
}

/* Parse a size_t array from a string like "1024,1024" or "1024" */
/* MISRA-C:2023 Rule 21.17: Replaced strtok with strtok_r */
static int ParseSizeArray(const char* str, size_t* arr, int max_count) {
    char buffer[MAX_BUFFER_LENGTH];
    char* saveptr = NULL;
    char* token;
    int count = 0;
    size_t val;

    if ((str == NULL) || (arr == NULL)) {
        return 0;
    }

    (void)strncpy(buffer, str, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    token = strtok_r(buffer, ",", &saveptr);
    while ((token != NULL) && (count < max_count)) {
        if (SafeStrToSize(Trim(token), &val)) {
            arr[count] = val;
            count++;
        } else {
            /* Conversion failed */
            return -1;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    return count;
}

/* Parse host type from string */
static HostType ParseHostType(const char* str) {
    if (str == NULL) {
        return HOST_TYPE_STANDARD; /* Default to standard */
    }

    if (strcmp(str, "cl_extension") == 0) {
        return HOST_TYPE_CL_EXTENSION;
    } else if (strcmp(str, "standard") == 0) {
        return HOST_TYPE_STANDARD;
    }

    return HOST_TYPE_STANDARD; /* Default to standard */
}

/* Parse buffer type from string */
static BufferType ParseBufferType(const char* str) {
    if (str == NULL) {
        return BUFFER_TYPE_NONE;
    }

    if (strcmp(str, "READ_ONLY") == 0) {
        return BUFFER_TYPE_READ_ONLY;
    } else if (strcmp(str, "WRITE_ONLY") == 0) {
        return BUFFER_TYPE_WRITE_ONLY;
    } else if (strcmp(str, "READ_WRITE") == 0) {
        return BUFFER_TYPE_READ_WRITE;
    }

    return BUFFER_TYPE_NONE;
}

/* Parse kernel argument type from string */
static KernelArgType ParseKernelArgType(const char* str) {
    if (str == NULL) {
        return KERNEL_ARG_TYPE_NONE;
    }

    if (strcmp(str, "input") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_INPUT;
    } else if (strcmp(str, "output") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_OUTPUT;
    } else if (strcmp(str, "buffer") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_CUSTOM;
    } else if (strcmp(str, "int") == 0) {
        return KERNEL_ARG_TYPE_SCALAR_INT;
    } else if (strcmp(str, "float") == 0) {
        return KERNEL_ARG_TYPE_SCALAR_FLOAT;
    } else if (strcmp(str, "size_t") == 0) {
        return KERNEL_ARG_TYPE_SCALAR_SIZE;
    }

    return KERNEL_ARG_TYPE_NONE;
}

/* Parse kernel arguments from string like "{int, src_width} {int, src_height}" */
static int ParseKernelArgs(const char* str, KernelArgDescriptor* args, int max_count) {
    char buffer[MAX_LINE_LENGTH * 2]; /* Larger buffer for long kernel_args strings */
    char* current;
    char* brace_start;
    char* brace_end;
    char* comma;
    char type_str[64];
    char source_str[64];
    int count = 0;

    if ((str == NULL) || (args == NULL)) {
        return 0;
    }

    (void)strncpy(buffer, str, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    current = buffer;

    /* Find each {...} block */
    while (count < max_count) {
        /* Find opening brace */
        brace_start = strchr(current, '{');
        if (brace_start == NULL) {
            break; /* No more arguments */
        }

        /* Find closing brace */
        brace_end = strchr(brace_start, '}');
        if (brace_end == NULL) {
            (void)fprintf(stderr, "Error: Unclosed brace in kernel_args\n");
            return -1;
        }

        /* Extract content between braces */
        *brace_end = '\0';
        char* content = Trim(brace_start + 1);

        /* Find comma separator */
        comma = strchr(content, ',');
        if (comma == NULL) {
            (void)fprintf(stderr, "Error: Missing comma in kernel_args: {%s}\n", content);
            return -1;
        }

        /* Split into type and source */
        *comma = '\0';
        (void)strncpy(type_str, Trim(content), sizeof(type_str) - 1U);
        type_str[sizeof(type_str) - 1U] = '\0';
        (void)strncpy(source_str, Trim(comma + 1), sizeof(source_str) - 1U);
        source_str[sizeof(source_str) - 1U] = '\0';

        /* Parse type */
        args[count].arg_type = ParseKernelArgType(type_str);
        if (args[count].arg_type == KERNEL_ARG_TYPE_NONE) {
            (void)fprintf(stderr, "Error: Invalid kernel arg type: %s\n", type_str);
            return -1;
        }

        /* Store source name */
        (void)strncpy(args[count].source_name, source_str, sizeof(args[count].source_name) - 1U);
        args[count].source_name[sizeof(args[count].source_name) - 1U] = '\0';

        count++;
        current = brace_end + 1;
    }

    return count;
}

/* Evaluate simple arithmetic expression (e.g., "1920 * 1080 * 4") */
static int EvalExpression(const char* str, size_t* result) {
    char buffer[MAX_BUFFER_LENGTH];
    char* saveptr = NULL;
    char* token;
    size_t value = 0;
    int has_value = 0;
    char operation = '\0';
    long temp_val;

    if ((str == NULL) || (result == NULL)) {
        return -1;
    }

    (void)strncpy(buffer, str, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    /* Simple tokenizer: split by spaces and handle *, +, - */
    token = strtok_r(buffer, " ", &saveptr);
    while (token != NULL) {
        if ((strcmp(token, "*") == 0) || (strcmp(token, "x") == 0)) {
            operation = '*';
        } else if (strcmp(token, "+") == 0) {
            operation = '+';
        } else if (strcmp(token, "-") == 0) {
            operation = '-';
        } else {
            /* Try to parse as number */
            if (!SafeStrtol(token, &temp_val) || (temp_val < 0)) {
                return -1;
            }

            if (!has_value) {
                value = (size_t)temp_val;
                has_value = 1;
            } else {
                /* Apply operation */
                if (operation == '*') {
                    value *= (size_t)temp_val;
                } else if (operation == '+') {
                    value += (size_t)temp_val;
                } else if (operation == '-') {
                    if (value >= (size_t)temp_val) {
                        value -= (size_t)temp_val;
                    } else {
                        return -1; /* Underflow */
                    }
                } else {
                    return -1; /* No operation specified */
                }
                operation = '\0';
            }
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    if (!has_value) {
        return -1;
    }

    *result = value;
    return 0;
}

int ParseConfig(const char* filename, Config* config) {
    FILE* fp;
    char line[MAX_LINE_LENGTH];
    char section[MAX_SECTION_LENGTH] = "";
    int kernel_index = -1;
    int buffer_index = -1;
    int scalar_index = -1;
    char* trimmed;
    char* equals;
    char* key;
    char* value;
    char* end;
    long temp_long;
    size_t temp_size;

    if ((filename == NULL) || (config == NULL)) {
        return -1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        (void)fprintf(stderr, "Error: Failed to open config file: %s\n", filename);
        return -1;
    }

    /* Initialize algorithm-specific fields (preserve input_images from ParseInputsConfig) */
    config->op_id[0] = '\0';
    config->dst_width = 0;
    config->dst_height = 0;
    config->dst_stride = 0;
    config->num_kernels = 0;
    config->custom_buffer_count = 0;
    config->scalar_arg_count = 0;
    /* Note: input_image_count and input_images[] preserved from ParseInputsConfig */

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        trimmed = Trim(line);

        /* Skip empty lines and comments */
        if ((trimmed[0] == '\0') || (trimmed[0] == '#') || (trimmed[0] == ';')) {
            continue;
        }

        /* Check for section header */
        if (trimmed[0] == '[') {
            end = strchr(trimmed, ']');
            if (end != NULL) {
                *end = '\0';
                (void)strncpy(section, trimmed + 1, sizeof(section) - 1U);
                section[sizeof(section) - 1U] = '\0';

                /* If section starts with "kernel.", it's a kernel configuration */
                if (strncmp(section, "kernel.", 7U) == 0) {
                    int variant_num;

                    kernel_index = config->num_kernels;
                    buffer_index = -1;
                    scalar_index = -1;
                    config->num_kernels++;
                    if (config->num_kernels > MAX_KERNEL_CONFIGS) {
                        (void)fprintf(stderr, "Error: Too many kernel configurations (max %d)\n",
                                      MAX_KERNEL_CONFIGS);
                        (void)fclose(fp);
                        return -1;
                    }
                    /* Parse variant_id from section name */
                    if (ParseKernelSection(section, config->kernels[kernel_index].variant_id,
                                           sizeof(config->kernels[kernel_index].variant_id)) != 0) {
                        (void)fprintf(stderr, "Error: Invalid kernel section name format: %s\n",
                                      section);
                        (void)fprintf(stderr, "Expected format: kernel.<variant_id>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                    /* Initialize host_type to default (standard) */
                    config->kernels[kernel_index].host_type = HOST_TYPE_STANDARD;
                    /* Auto-derive kernel_variant from variant_id (e.g., "v0" -> 0, "v1"
                     * -> 1) */
                    variant_num = ExtractVariantNumber(config->kernels[kernel_index].variant_id);
                    if (variant_num < 0) {
                        (void)fprintf(stderr,
                                      "Error: Invalid variant_id format: %s (expected v0, "
                                      "v1, v2, ...)\n",
                                      config->kernels[kernel_index].variant_id);
                        (void)fclose(fp);
                        return -1;
                    }
                    config->kernels[kernel_index].kernel_variant = variant_num;
                }
                /* If section starts with "buffer.", it's a custom buffer configuration
                 */
                else if (strncmp(section, "buffer.", 7U) == 0) {
                    buffer_index = config->custom_buffer_count;
                    kernel_index = -1;
                    scalar_index = -1;
                    config->custom_buffer_count++;
                    if (config->custom_buffer_count > MAX_CUSTOM_BUFFERS) {
                        (void)fprintf(stderr, "Error: Too many custom buffers (max %d)\n",
                                      MAX_CUSTOM_BUFFERS);
                        (void)fclose(fp);
                        return -1;
                    }
                    /* Zero-initialize the new buffer entry */
                    (void)memset(&config->custom_buffers[buffer_index], 0,
                                 sizeof(CustomBufferConfig));
                    /* Parse buffer name from section name */
                    if (ParseBufferSection(section, config->custom_buffers[buffer_index].name,
                                           sizeof(config->custom_buffers[buffer_index].name)) !=
                        0) {
                        (void)fprintf(stderr, "Error: Invalid buffer section name format: %s\n",
                                      section);
                        (void)fprintf(stderr, "Expected format: buffer.<name>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                }
                /* If section starts with "scalar.", it's a scalar argument
                   configuration */
                else if (strncmp(section, "scalar.", 7U) == 0) {
                    scalar_index = config->scalar_arg_count;
                    kernel_index = -1;
                    buffer_index = -1;
                    config->scalar_arg_count++;
                    if (config->scalar_arg_count > MAX_SCALAR_ARGS) {
                        (void)fprintf(stderr, "Error: Too many scalar arguments (max %d)\n",
                                      MAX_SCALAR_ARGS);
                        (void)fclose(fp);
                        return -1;
                    }
                    /* Parse scalar name from section name */
                    if (ParseScalarSection(section, config->scalar_args[scalar_index].name,
                                           sizeof(config->scalar_args[scalar_index].name)) != 0) {
                        (void)fprintf(stderr, "Error: Invalid scalar section name format: %s\n",
                                      section);
                        (void)fprintf(stderr, "Expected format: scalar.<name>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                } else {
                    /* Other sections like [image] - reset indices */
                    kernel_index = -1;
                    buffer_index = -1;
                    scalar_index = -1;
                }
            }
            continue;
        }

        /* Parse key=value pairs */
        equals = strchr(trimmed, '=');
        if (equals == NULL) {
            continue;
        }

        *equals = '\0';
        key = Trim(trimmed);
        value = Trim(equals + 1);

        /* Strip inline comments from value (everything after #) */
        {
            char* comment = strchr(value, '#');
            if (comment != NULL) {
                *comment = '\0';
                value = Trim(value); /* Trim again after removing comment */
            }
        }

        /* Parse based on section */
        if ((strcmp(section, "image") == 0) || (strcmp(section, "output") == 0)) {
            if (strcmp(key, "op_id") == 0) {
                (void)strncpy(config->op_id, value, sizeof(config->op_id) - 1U);
                config->op_id[sizeof(config->op_id) - 1U] = '\0';
            } else if (strcmp(key, "dst_width") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    config->dst_width = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid dst_width value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "dst_height") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    config->dst_height = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid dst_height value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "dst_stride") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    config->dst_stride = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid dst_stride value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else {
                /* Unknown key in image/output section - ignore */
            }
        } else if ((strncmp(section, "kernel.", 7U) == 0) && (kernel_index >= 0)) {
            KernelConfig* kc = &config->kernels[kernel_index];

            if (strcmp(key, "kernel_file") == 0) {
                (void)strncpy(kc->kernel_file, value, sizeof(kc->kernel_file) - 1U);
                kc->kernel_file[sizeof(kc->kernel_file) - 1U] = '\0';
            } else if (strcmp(key, "kernel_function") == 0) {
                (void)strncpy(kc->kernel_function, value, sizeof(kc->kernel_function) - 1U);
                kc->kernel_function[sizeof(kc->kernel_function) - 1U] = '\0';
            } else if (strcmp(key, "work_dim") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    kc->work_dim = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid work_dim value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "global_work_size") == 0) {
                if (ParseSizeArray(value, kc->global_work_size, 3) < 0) {
                    (void)fprintf(stderr, "Error: Invalid global_work_size: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "local_work_size") == 0) {
                if (ParseSizeArray(value, kc->local_work_size, 3) < 0) {
                    (void)fprintf(stderr, "Error: Invalid local_work_size: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "host_type") == 0) {
                kc->host_type = ParseHostType(value);
            } else if (strcmp(key, "kernel_args") == 0) {
                /* Parse kernel arguments */
                int arg_count = ParseKernelArgs(value, kc->kernel_args, MAX_KERNEL_ARGS);
                if (arg_count < 0) {
                    (void)fprintf(stderr, "Error: Failed to parse kernel_args\n");
                    (void)fclose(fp);
                    return -1;
                }
                kc->kernel_arg_count = arg_count;
            } else {
                /* Unknown key in kernel section */
                /* Note: kernel_variant is auto-derived from section name (e.g.,
                 * [kernel.v0] -> variant 0) */
            }
        } else if ((strncmp(section, "buffer.", 7U) == 0) && (buffer_index >= 0)) {
            CustomBufferConfig* buf = &config->custom_buffers[buffer_index];

            if (strcmp(key, "type") == 0) {
                buf->type = ParseBufferType(value);
                if (buf->type == BUFFER_TYPE_NONE) {
                    (void)fprintf(stderr, "Error: Invalid buffer type: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "data_type") == 0) {
                buf->data_type = ParseDataType(value);
                if (buf->data_type == DATA_TYPE_NONE) {
                    (void)fprintf(stderr, "Error: Invalid data type: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "num_elements") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    buf->num_elements = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid num_elements value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "source_file") == 0) {
                (void)strncpy(buf->source_file, value, sizeof(buf->source_file) - 1U);
                buf->source_file[sizeof(buf->source_file) - 1U] = '\0';
            } else if (strcmp(key, "size_bytes") == 0) {
                if (EvalExpression(value, &temp_size) == 0) {
                    buf->size_bytes = temp_size;
                } else {
                    (void)fprintf(stderr, "Error: Invalid size_bytes expression: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else {
                /* Unknown key in buffer section */
            }
        } else if ((strncmp(section, "scalar.", 7U) == 0) && (scalar_index >= 0)) {
            ScalarArgConfig* scalar = &config->scalar_args[scalar_index];

            if (strcmp(key, "value") == 0) {
                if (SafeStrtol(value, &temp_long)) {
                    scalar->value = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid scalar value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else {
                /* Unknown key in scalar section */
            }
        } else {
            /* Unknown section or invalid index */
        }
    }

    (void)fclose(fp);

    /* Validate custom buffers */
    {
        int i;
        for (i = 0; i < config->custom_buffer_count; i++) {
            CustomBufferConfig* buf = &config->custom_buffers[i];

            /* Check if buffer type is set */
            if (buf->type == BUFFER_TYPE_NONE) {
                (void)fprintf(stderr, "Error: Buffer '%s' missing 'type' field\n", buf->name);
                return -1;
            }

            /* File-backed buffer: must have source_file, data_type, and num_elements
             */
            if (buf->source_file[0] != '\0') {
                if (buf->data_type == DATA_TYPE_NONE) {
                    (void)fprintf(stderr,
                                  "Error: File-backed buffer '%s' missing 'data_type' field\n",
                                  buf->name);
                    return -1;
                }
                if (buf->num_elements == 0) {
                    (void)fprintf(stderr,
                                  "Error: File-backed buffer '%s' missing 'num_elements' field\n",
                                  buf->name);
                    return -1;
                }
                /* Calculate size_bytes from data_type and num_elements */
                buf->size_bytes = GetDataTypeSize(buf->data_type) * (size_t)buf->num_elements;
            }
            /* Empty buffer: must have size_bytes */
            else {
                if (buf->size_bytes == 0) {
                    (void)fprintf(stderr, "Error: Empty buffer '%s' missing 'size_bytes' field\n",
                                  buf->name);
                    return -1;
                }
            }
        }
    }

    /* Note: op_id is now optional - can be auto-derived from filename in main.c
     */
    if (config->op_id[0] != '\0') {
        (void)printf(
            "Parsed config file: %s (op_id: %s, %d kernels, %d custom buffers, %d "
            "scalars)\n",
            filename, config->op_id, config->num_kernels, config->custom_buffer_count,
            config->scalar_arg_count);
    } else {
        (void)printf("Parsed config file: %s (%d kernels, %d custom buffers, %d scalars)\n",
                     filename, config->num_kernels, config->custom_buffer_count,
                     config->scalar_arg_count);
    }
    return 0;
}

/* MISRA-C:2023 Rule 8.14: Removed static variable for reentrancy */
int GetOpVariants(const Config* config, const char* op_id, KernelConfig* variants[], int* count) {
    int i;

    if ((config == NULL) || (op_id == NULL) || (variants == NULL) || (count == NULL)) {
        return -1;
    }

    /* Check if the config's op_id matches the requested op_id */
    if (strcmp(config->op_id, op_id) != 0) {
        /* Operation not found in this config */
        *count = 0;
        return 0;
    }

    /* All kernels in the config are variants of this operation */
    for (i = 0; i < config->num_kernels; i++) {
        /* Casting away const for backwards compatibility */
        /* In a stricter implementation, Config should not be const */
        /* or variants should be const KernelConfig* */
        variants[i] = (KernelConfig*)&config->kernels[i];
    }

    *count = config->num_kernels;
    return 0;
}

int ResolveConfigPath(const char* input, char* output, size_t output_size) {
    FILE* test_file;
    int written;

    if ((input == NULL) || (output == NULL) || (output_size == 0U)) {
        return -1;
    }

    /* Treat input as algorithm name - construct config/<name>.ini */
    written = snprintf(output, output_size, "config/%s.ini", input);
    if ((written < 0) || ((size_t)written >= output_size)) {
        return -1;
    }

    /* Verify file exists */
    test_file = fopen(output, "r");
    if (test_file == NULL) {
        (void)fprintf(stderr, "Config file not found: %s\n", output);
        return -1;
    }
    (void)fclose(test_file);

    return 0;
}

int ExtractOpIdFromPath(const char* config_path, char* op_id, size_t op_id_size) {
    const char* last_slash;
    const char* last_dot;
    const char* filename;
    size_t name_len;

    if ((config_path == NULL) || (op_id == NULL) || (op_id_size == 0U)) {
        return -1;
    }

    /* Find filename (after last slash) */
    last_slash = strrchr(config_path, '/');
    filename = (last_slash != NULL) ? (last_slash + 1) : config_path;

    /* Find extension (last dot) */
    last_dot = strrchr(filename, '.');
    if (last_dot == NULL) {
        /* No extension, use entire filename */
        if (strlen(filename) >= op_id_size) {
            return -1;
        }
        (void)strncpy(op_id, filename, op_id_size - 1U);
        op_id[op_id_size - 1U] = '\0';
    } else {
        /* Extract name before extension */
        name_len = (size_t)(last_dot - filename);
        if (name_len >= op_id_size) {
            return -1;
        }
        (void)strncpy(op_id, filename, name_len);
        op_id[name_len] = '\0';
    }

    return 0;
}

/* Parse image section name to extract image index */
/* Format: "image_N" where N is 1, 2, 3, etc. */
static int ParseImageSection(const char* section, int* image_index) {
    const char* underscore;
    long temp_long;

    if ((section == NULL) || (image_index == NULL)) {
        return -1;
    }

    /* Check if section starts with "image_" */
    if (strncmp(section, "image_", 6U) != 0) {
        return -1;
    }

    underscore = section + 6; /* Points to start of index */

    /* Parse the index number */
    if (!SafeStrtol(underscore, &temp_long) || (temp_long < 1) || (temp_long > MAX_INPUT_IMAGES)) {
        return -1;
    }

    /* Convert to 0-based index */
    *image_index = (int)temp_long - 1;
    return 0;
}

int ParseInputsConfig(const char* filename, Config* config) {
    FILE* fp;
    char line[MAX_LINE_LENGTH];
    char section[MAX_SECTION_LENGTH] = "";
    int current_image_index = -1;
    char* trimmed;
    char* equals;
    char* key;
    char* value;
    char* end;
    long temp_long;
    size_t temp_size;

    if ((filename == NULL) || (config == NULL)) {
        return -1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        (void)fprintf(stderr, "Error: Failed to open inputs config file: %s\n", filename);
        return -1;
    }

    /* Initialize input images count */
    config->input_image_count = 0;

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        trimmed = Trim(line);

        /* Skip empty lines and comments */
        if ((trimmed[0] == '\0') || (trimmed[0] == '#') || (trimmed[0] == ';')) {
            continue;
        }

        /* Check for section header */
        if (trimmed[0] == '[') {
            end = strchr(trimmed, ']');
            if (end != NULL) {
                *end = '\0';
                (void)strncpy(section, trimmed + 1, sizeof(section) - 1U);
                section[sizeof(section) - 1U] = '\0';

                /* If section starts with "image_", it's an input image configuration */
                if (strncmp(section, "image_", 6U) == 0) {
                    if (ParseImageSection(section, &current_image_index) != 0) {
                        (void)fprintf(stderr, "Error: Invalid image section name: %s\n", section);
                        (void)fprintf(stderr, "Expected format: image_1, image_2, etc.\n");
                        (void)fclose(fp);
                        return -1;
                    }

                    /* Update image count */
                    if (current_image_index + 1 > config->input_image_count) {
                        config->input_image_count = current_image_index + 1;
                    }

                    /* Initialize this image config */
                    (void)memset(&config->input_images[current_image_index], 0,
                                 sizeof(InputImageConfig));
                } else {
                    /* Unknown section */
                    current_image_index = -1;
                }
            }
            continue;
        }

        /* Parse key=value pairs */
        equals = strchr(trimmed, '=');
        if (equals == NULL) {
            continue;
        }

        *equals = '\0';
        key = Trim(trimmed);
        value = Trim(equals + 1);

        /* Strip inline comments from value (everything after #) */
        {
            char* comment = strchr(value, '#');
            if (comment != NULL) {
                *comment = '\0';
                value = Trim(value); /* Trim again after removing comment */
            }
        }

        /* Parse based on current section */
        if ((strncmp(section, "image_", 6U) == 0) && (current_image_index >= 0)) {
            InputImageConfig* img = &config->input_images[current_image_index];

            if (strcmp(key, "input") == 0) {
                (void)strncpy(img->input_path, value, sizeof(img->input_path) - 1U);
                img->input_path[sizeof(img->input_path) - 1U] = '\0';
            } else if (strcmp(key, "src_width") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    img->src_width = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_width value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "src_height") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    img->src_height = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_height value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "src_channels") == 0) {
                if (SafeStrtol(value, &temp_long) && (temp_long > 0)) {
                    img->src_channels = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_channels value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "src_stride") == 0) {
                /* Support arithmetic expressions like "1920 * 3" */
                if (EvalExpression(value, &temp_size) == 0) {
                    img->src_stride = (int)temp_size;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_stride expression: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else {
                /* Unknown key in image section */
            }
        } else {
            /* Unknown section or invalid index */
        }
    }

    (void)fclose(fp);

    (void)printf("Parsed inputs config file: %s (%d input images)\n", filename,
                 config->input_image_count);
    return 0;
}
