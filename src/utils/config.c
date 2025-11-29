/* POSIX feature test macro for strtok_r */
#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "safe_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Maximum line length for config file */
#define MAX_LINE_LENGTH 512
#define MAX_SECTION_LENGTH 64
#define MAX_BUFFER_LENGTH 256

/* Helper function to trim whitespace */
static char* trim(char* str) {
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
static int parse_kernel_section(const char* section, char* variant_id,
                                size_t variant_id_size) {
    const char* first_dot;
    size_t variant_len;

    if ((section == NULL) || (variant_id == NULL)) {
        return -1;
    }

    /* Skip "kernel." prefix */
    if (strncmp(section, "kernel.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7;  /* Points to start of variant_id */

    /* Extract variant_id */
    variant_len = strlen(first_dot);
    if ((variant_len == 0U) || (variant_len >= variant_id_size)) {
        return -1;  /* variant_id empty or too long */
    }
    (void)strncpy(variant_id, first_dot, variant_id_size - 1U);
    variant_id[variant_id_size - 1U] = '\0';

    return 0;
}

/* Parse buffer section name to extract buffer name */
/* Format: "buffer.<name>" */
static int parse_buffer_section(const char* section, char* name, size_t name_size) {
    const char* first_dot;
    size_t name_len;

    if ((section == NULL) || (name == NULL)) {
        return -1;
    }

    /* Skip "buffer." prefix */
    if (strncmp(section, "buffer.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7;  /* Points to start of buffer name */

    /* Extract buffer name */
    name_len = strlen(first_dot);
    if ((name_len == 0U) || (name_len >= name_size)) {
        return -1;  /* name empty or too long */
    }
    (void)strncpy(name, first_dot, name_size - 1U);
    name[name_size - 1U] = '\0';

    return 0;
}

/* Parse scalar section name to extract scalar name */
/* Format: "scalar.<name>" */
static int parse_scalar_section(const char* section, char* name, size_t name_size) {
    const char* first_dot;
    size_t name_len;

    if ((section == NULL) || (name == NULL)) {
        return -1;
    }

    /* Skip "scalar." prefix */
    if (strncmp(section, "scalar.", 7U) != 0) {
        return -1;
    }

    first_dot = section + 7;  /* Points to start of scalar name */

    /* Extract scalar name */
    name_len = strlen(first_dot);
    if ((name_len == 0U) || (name_len >= name_size)) {
        return -1;  /* name empty or too long */
    }
    (void)strncpy(name, first_dot, name_size - 1U);
    name[name_size - 1U] = '\0';

    return 0;
}

/* Parse data type from string */
static DataType parse_data_type(const char* str) {
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
static size_t get_data_type_size(DataType type) {
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
static int parse_size_array(const char* str, size_t* arr, int max_count) {
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
        if (safe_str_to_size(trim(token), &val)) {
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
static HostType parse_host_type(const char* str) {
    if (str == NULL) {
        return HOST_TYPE_STANDARD;  /* Default to standard */
    }

    if (strcmp(str, "cl_extension") == 0) {
        return HOST_TYPE_CL_EXTENSION;
    } else if (strcmp(str, "standard") == 0) {
        return HOST_TYPE_STANDARD;
    }

    return HOST_TYPE_STANDARD;  /* Default to standard */
}

/* Parse buffer type from string */
static BufferType parse_buffer_type(const char* str) {
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

/* Parse buffer types array from comma-separated string */
static int parse_buffer_types(const char* str, BufferType* arr, int max_count) {
    char buffer[MAX_BUFFER_LENGTH];
    char* saveptr = NULL;
    char* token;
    int count = 0;

    if ((str == NULL) || (arr == NULL)) {
        return 0;
    }

    (void)strncpy(buffer, str, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    token = strtok_r(buffer, ",", &saveptr);
    while ((token != NULL) && (count < max_count)) {
        arr[count] = parse_buffer_type(trim(token));
        if (arr[count] == BUFFER_TYPE_NONE) {
            (void)fprintf(stderr, "Warning: Invalid buffer type: %s\n", trim(token));
        }
        count++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    return count;
}

/* Evaluate simple arithmetic expression (e.g., "1920 * 1080 * 4") */
static int eval_expression(const char* str, size_t* result) {
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
            if (!safe_strtol(token, &temp_val) || (temp_val < 0)) {
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
                        return -1;  /* Underflow */
                    }
                } else {
                    return -1;  /* No operation specified */
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

/* Parse buffer sizes array from comma-separated expressions */
static int parse_buffer_sizes(const char* str, size_t* arr, int max_count) {
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
        if (eval_expression(trim(token), &val) == 0) {
            arr[count] = val;
            count++;
        } else {
            (void)fprintf(stderr, "Error: Invalid buffer size expression: %s\n", trim(token));
            return -1;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    return count;
}

int parse_config(const char* filename, Config* config) {
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

    /* Initialize config */
    (void)memset(config, 0, sizeof(Config));
    config->num_kernels = 0;
    config->custom_buffer_count = 0;
    config->scalar_arg_count = 0;

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        trimmed = trim(line);

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
                    if (parse_kernel_section(section,
                                            config->kernels[kernel_index].variant_id,
                                            sizeof(config->kernels[kernel_index].variant_id)) != 0) {
                        (void)fprintf(stderr, "Error: Invalid kernel section name format: %s\n", section);
                        (void)fprintf(stderr, "Expected format: kernel.<variant_id>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                    /* Initialize host_type to default (standard) */
                    config->kernels[kernel_index].host_type = HOST_TYPE_STANDARD;
                }
                /* If section starts with "buffer.", it's a custom buffer configuration */
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
                    /* Parse buffer name from section name */
                    if (parse_buffer_section(section,
                                            config->custom_buffers[buffer_index].name,
                                            sizeof(config->custom_buffers[buffer_index].name)) != 0) {
                        (void)fprintf(stderr, "Error: Invalid buffer section name format: %s\n", section);
                        (void)fprintf(stderr, "Expected format: buffer.<name>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                }
                /* If section starts with "scalar.", it's a scalar argument configuration */
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
                    if (parse_scalar_section(section,
                                            config->scalar_args[scalar_index].name,
                                            sizeof(config->scalar_args[scalar_index].name)) != 0) {
                        (void)fprintf(stderr, "Error: Invalid scalar section name format: %s\n", section);
                        (void)fprintf(stderr, "Expected format: scalar.<name>\n");
                        (void)fclose(fp);
                        return -1;
                    }
                }
                else {
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
        key = trim(trimmed);
        value = trim(equals + 1);

        /* Strip inline comments from value (everything after #) */
        {
            char* comment = strchr(value, '#');
            if (comment != NULL) {
                *comment = '\0';
                value = trim(value);  /* Trim again after removing comment */
            }
        }

        /* Parse based on section */
        if (strcmp(section, "image") == 0) {
            if (strcmp(key, "op_id") == 0) {
                (void)strncpy(config->op_id, value, sizeof(config->op_id) - 1U);
                config->op_id[sizeof(config->op_id) - 1U] = '\0';
            } else if (strcmp(key, "input") == 0) {
                (void)strncpy(config->input_image, value, sizeof(config->input_image) - 1U);
                config->input_image[sizeof(config->input_image) - 1U] = '\0';
            } else if (strcmp(key, "output") == 0) {
                (void)strncpy(config->output_image, value, sizeof(config->output_image) - 1U);
                config->output_image[sizeof(config->output_image) - 1U] = '\0';
            } else if (strcmp(key, "src_width") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->src_width = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_width value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "src_height") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->src_height = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid src_height value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "dst_width") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->dst_width = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid dst_width value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "dst_height") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->dst_height = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid dst_height value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "kernel_x_file") == 0) {
                (void)strncpy(config->kernel_x_file, value, sizeof(config->kernel_x_file) - 1U);
                config->kernel_x_file[sizeof(config->kernel_x_file) - 1U] = '\0';
            } else if (strcmp(key, "kernel_y_file") == 0) {
                (void)strncpy(config->kernel_y_file, value, sizeof(config->kernel_y_file) - 1U);
                config->kernel_y_file[sizeof(config->kernel_y_file) - 1U] = '\0';
            } else if (strcmp(key, "cl_buffer_type") == 0) {
                int count = parse_buffer_types(value, config->cl_buffer_type, MAX_CUSTOM_BUFFERS);
                if (count < 0) {
                    (void)fprintf(stderr, "Error: Invalid cl_buffer_type: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
                (void)printf("Parsed %d buffer types: ", count);
                {
                    int i;
                    for (i = 0; i < count; i++) {
                        const char* type_name = "UNKNOWN";
                        if (config->cl_buffer_type[i] == BUFFER_TYPE_READ_ONLY) {
                            type_name = "READ_ONLY";
                        } else if (config->cl_buffer_type[i] == BUFFER_TYPE_WRITE_ONLY) {
                            type_name = "WRITE_ONLY";
                        } else if (config->cl_buffer_type[i] == BUFFER_TYPE_READ_WRITE) {
                            type_name = "READ_WRITE";
                        }
                        (void)printf("%s%s", type_name, (i < count - 1) ? ", " : "\n");
                    }
                }
            } else if (strcmp(key, "cl_buffer_size") == 0) {
                int count = parse_buffer_sizes(value, config->cl_buffer_size, MAX_CUSTOM_BUFFERS);
                if (count < 0) {
                    (void)fprintf(stderr, "Error: Invalid cl_buffer_size: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
                config->num_buffers = count;
                (void)printf("Parsed %d buffer sizes: ", count);
                {
                    int i;
                    for (i = 0; i < count; i++) {
                        (void)printf("%zu%s", config->cl_buffer_size[i],
                                   (i < count - 1) ? ", " : "\n");
                    }
                }
            } else {
                /* Unknown key in image section */
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
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    kc->work_dim = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid work_dim value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "global_work_size") == 0) {
                if (parse_size_array(value, kc->global_work_size, 3) < 0) {
                    (void)fprintf(stderr, "Error: Invalid global_work_size: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "local_work_size") == 0) {
                if (parse_size_array(value, kc->local_work_size, 3) < 0) {
                    (void)fprintf(stderr, "Error: Invalid local_work_size: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "host_type") == 0) {
                kc->host_type = parse_host_type(value);
            } else {
                /* Unknown key in kernel section */
            }
        } else if ((strncmp(section, "buffer.", 7U) == 0) && (buffer_index >= 0)) {
            CustomBufferConfig* buf = &config->custom_buffers[buffer_index];

            if (strcmp(key, "type") == 0) {
                buf->type = parse_buffer_type(value);
                if (buf->type == BUFFER_TYPE_NONE) {
                    (void)fprintf(stderr, "Error: Invalid buffer type: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "data_type") == 0) {
                buf->data_type = parse_data_type(value);
                if (buf->data_type == DATA_TYPE_NONE) {
                    (void)fprintf(stderr, "Error: Invalid data type: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "num_elements") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
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
                if (eval_expression(value, &temp_size) == 0) {
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
                if (safe_strtol(value, &temp_long)) {
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

            /* File-backed buffer: must have source_file, data_type, and num_elements */
            if (buf->source_file[0] != '\0') {
                if (buf->data_type == DATA_TYPE_NONE) {
                    (void)fprintf(stderr, "Error: File-backed buffer '%s' missing 'data_type' field\n", buf->name);
                    return -1;
                }
                if (buf->num_elements == 0) {
                    (void)fprintf(stderr, "Error: File-backed buffer '%s' missing 'num_elements' field\n", buf->name);
                    return -1;
                }
                /* Calculate size_bytes from data_type and num_elements */
                buf->size_bytes = get_data_type_size(buf->data_type) * (size_t)buf->num_elements;
            }
            /* Empty buffer: must have size_bytes */
            else {
                if (buf->size_bytes == 0) {
                    (void)fprintf(stderr, "Error: Empty buffer '%s' missing 'size_bytes' field\n", buf->name);
                    return -1;
                }
            }
        }
    }

    /* Note: op_id is now optional - can be auto-derived from filename in main.c */
    if (config->op_id[0] != '\0') {
        (void)printf("Parsed config file: %s (op_id: %s, %d kernels, %d custom buffers, %d scalars)\n",
                     filename, config->op_id, config->num_kernels,
                     config->custom_buffer_count, config->scalar_arg_count);
    } else {
        (void)printf("Parsed config file: %s (%d kernels, %d custom buffers, %d scalars)\n",
                     filename, config->num_kernels,
                     config->custom_buffer_count, config->scalar_arg_count);
    }
    return 0;
}

/* MISRA-C:2023 Rule 8.14: Removed static variable for reentrancy */
int get_op_variants(const Config* config, const char* op_id,
                    KernelConfig* variants[], int* count) {
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
