/* POSIX feature test macro for strtok_r */
#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "utils/safe_ops.h"

/* Maximum line length for config file */
#define MAX_LINE_LENGTH 512
#define MAX_BUFFER_LENGTH 256

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
/* Using static buffer with maximum size constraint for config files */
#define MAX_CONFIG_FILE_SIZE (64 * 1024) /* 64KB max config file */
static char config_file_buffer[MAX_CONFIG_FILE_SIZE];

/* Helper function to read entire file into string */
static char* ReadFileToString(const char* filename) {
    FILE* fp;
    long file_size;
    size_t bytes_read;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        (void)fclose(fp);
        return NULL;
    }
    file_size = ftell(fp);
    if (file_size < 0) {
        (void)fclose(fp);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        (void)fclose(fp);
        return NULL;
    }

    /* Check if file fits in static buffer */
    if ((size_t)file_size >= MAX_CONFIG_FILE_SIZE) {
        (void)fprintf(stderr, "Error: Config file too large (%ld bytes, max %d)\n",
                      file_size, MAX_CONFIG_FILE_SIZE - 1);
        (void)fclose(fp);
        return NULL;
    }

    /* Read file into static buffer */
    bytes_read = fread(config_file_buffer, 1U, (size_t)file_size, fp);
    config_file_buffer[bytes_read] = '\0';

    (void)fclose(fp);
    return config_file_buffer;
}

/* Extract numeric variant number from variant_id */
/* Format: "v0" -> 0, "v1" -> 1, "v1f" -> 1, "v10" -> 10, etc. */
/* Supports alphanumeric suffixes - extracts leading digits only */
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

    /* Skip 'v' prefix and parse the leading digits */
    temp_long = strtol(variant_id + 1, &endptr, 10);

    /* Validate conversion - require at least one digit was parsed */
    /* Allow trailing alpha chars (e.g., "1f", "2f") for alphanumeric variants */
    if ((endptr == (variant_id + 1)) || (temp_long < 0) || (temp_long > 99)) {
        return -1; /* No digits parsed or number out of range */
    }

    return (int)temp_long;
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

/* Parse host type from string */
static HostType ParseHostType(const char* str) {
    if (str == NULL) {
        return HOST_TYPE_CL_EXTENSION; /* Default to cl_extension */
    }

    if (strcmp(str, "standard") == 0) {
        return HOST_TYPE_STANDARD;
    } else if (strcmp(str, "cl_extension") == 0) {
        return HOST_TYPE_CL_EXTENSION;
    }

    return HOST_TYPE_CL_EXTENSION; /* Default to cl_extension */
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

/* Parse scalar type from string */
static ScalarType ParseScalarTypeStr(const char* str) {
    if (str == NULL) {
        return SCALAR_TYPE_NONE;
    }

    if (strcmp(str, "int") == 0) {
        return SCALAR_TYPE_INT;
    } else if (strcmp(str, "float") == 0) {
        return SCALAR_TYPE_FLOAT;
    } else if ((strcmp(str, "size_t") == 0) || (strcmp(str, "size") == 0)) {
        return SCALAR_TYPE_SIZE;
    }

    return SCALAR_TYPE_NONE;
}

/* Parse kernel argument type from key name (new format) */
static KernelArgType ParseKernelArgKey(const char* key) {
    if (key == NULL) {
        return KERNEL_ARG_TYPE_NONE;
    }

    if (strcmp(key, "i_buffer") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_INPUT;
    } else if (strcmp(key, "o_buffer") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_OUTPUT;
    } else if (strcmp(key, "buffer") == 0) {
        return KERNEL_ARG_TYPE_BUFFER_CUSTOM;
    } else if (strcmp(key, "param") == 0) {
        /* param type will be determined by data_type in array */
        return KERNEL_ARG_TYPE_SCALAR_INT; /* placeholder, will be updated */
    }

    return KERNEL_ARG_TYPE_NONE;
}

/* Get scalar arg type from data type string */
static KernelArgType GetScalarArgType(const char* data_type_str) {
    if (data_type_str == NULL) {
        return KERNEL_ARG_TYPE_NONE;
    }

    if (strcmp(data_type_str, "int") == 0) {
        return KERNEL_ARG_TYPE_SCALAR_INT;
    } else if (strcmp(data_type_str, "float") == 0) {
        return KERNEL_ARG_TYPE_SCALAR_FLOAT;
    } else if ((strcmp(data_type_str, "size_t") == 0) || (strcmp(data_type_str, "size") == 0)) {
        return KERNEL_ARG_TYPE_SCALAR_SIZE;
    }

    return KERNEL_ARG_TYPE_NONE;
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

/* Helper to get integer from JSON (supports both number and string expression) */
static int GetJsonInt(const cJSON* json, const char* key, int* value) {
    cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (item == NULL) {
        return -1;
    }

    if (cJSON_IsNumber(item)) {
        *value = item->valueint;
        return 0;
    } else if (cJSON_IsString(item)) {
        size_t temp;
        if (EvalExpression(item->valuestring, &temp) == 0) {
            *value = (int)temp;
            return 0;
        }
    }
    return -1;
}

/* Helper to get size_t from JSON (supports both number and string expression) */
static int GetJsonSize(const cJSON* json, const char* key, size_t* value) {
    cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (item == NULL) {
        return -1;
    }

    if (cJSON_IsNumber(item)) {
        *value = (size_t)item->valuedouble;
        return 0;
    } else if (cJSON_IsString(item)) {
        return EvalExpression(item->valuestring, value);
    }
    return -1;
}

/* Helper to get string from JSON */
static int GetJsonString(const cJSON* json, const char* key, char* buffer, size_t buffer_size) {
    cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if ((item == NULL) || !cJSON_IsString(item)) {
        return -1;
    }

    (void)strncpy(buffer, item->valuestring, buffer_size - 1U);
    buffer[buffer_size - 1U] = '\0';
    return 0;
}

/* Helper to get float from JSON */
static int GetJsonFloat(const cJSON* json, const char* key, float* value) {
    cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (item == NULL) {
        return -1;
    }

    if (cJSON_IsNumber(item)) {
        *value = (float)item->valuedouble;
        return 0;
    }
    return -1;
}

/* Parse kernel arguments from JSON array
 * New format: {"key": ["data_type", "name"]} or {"key": ["data_type", "name", size]}
 * - i_buffer: Input buffer  (e.g., {"i_buffer": ["uchar", "src"]})
 * - o_buffer: Output buffer (e.g., {"o_buffer": ["uchar", "dst"]})
 * - buffer:   Custom buffer (e.g., {"buffer": ["uchar", "tmp", 45000]})
 * - param:    Scalar param  (e.g., {"param": ["int", "src_width"]})
 * - struct:   Packed struct (e.g., {"struct": ["field1", "field2", ...]})
 */
static int ParseKernelArgsJson(const cJSON* args_array, KernelArgDescriptor* args, int max_count) {
    const cJSON* arg;
    int count = 0;
    const char* arg_keys[] = {"i_buffer", "o_buffer", "buffer", "param", "struct"};
    int num_keys = 5;

    if ((args_array == NULL) || !cJSON_IsArray(args_array)) {
        return 0;
    }

    cJSON_ArrayForEach(arg, args_array) {
        cJSON* value_array = NULL;
        const char* matched_key = NULL;
        int i;
        int array_size;

        if (count >= max_count) {
            (void)fprintf(stderr, "Error: Too many kernel arguments (max %d)\n", max_count);
            return -1;
        }

        if (!cJSON_IsObject(arg)) {
            (void)fprintf(stderr, "Error: Kernel argument must be an object\n");
            return -1;
        }

        /* Find which key is present (i_buffer, o_buffer, buffer, or param) */
        for (i = 0; i < num_keys; i++) {
            value_array = cJSON_GetObjectItemCaseSensitive(arg, arg_keys[i]);
            if (value_array != NULL) {
                matched_key = arg_keys[i];
                break;
            }
        }

        if ((matched_key == NULL) || (value_array == NULL)) {
            (void)fprintf(stderr,
                          "Error: Kernel argument must have one of: i_buffer, o_buffer, buffer, param, struct\n");
            return -1;
        }

        /* Initialize struct fields */
        args[count].struct_field_count = 0;

        /* Handle struct specially - it's an array of field names */
        if (strcmp(matched_key, "struct") == 0) {
            int field_count;
            int j;

            if (!cJSON_IsArray(value_array)) {
                (void)fprintf(stderr, "Error: 'struct' value must be an array of field names\n");
                return -1;
            }

            field_count = cJSON_GetArraySize(value_array);
            if (field_count < 1) {
                (void)fprintf(stderr, "Error: 'struct' must have at least one field\n");
                return -1;
            }
            if (field_count > MAX_STRUCT_FIELDS) {
                (void)fprintf(stderr, "Error: 'struct' has too many fields (max %d)\n",
                              MAX_STRUCT_FIELDS);
                return -1;
            }

            args[count].arg_type = KERNEL_ARG_TYPE_STRUCT;
            args[count].struct_field_count = field_count;

            /* Copy field names */
            for (j = 0; j < field_count; j++) {
                cJSON* field = cJSON_GetArrayItem(value_array, j);
                if (!cJSON_IsString(field)) {
                    (void)fprintf(stderr, "Error: 'struct' field %d must be a string\n", j);
                    return -1;
                }
                (void)strncpy(args[count].struct_fields[j], field->valuestring,
                              sizeof(args[count].struct_fields[j]) - 1U);
                args[count].struct_fields[j][sizeof(args[count].struct_fields[j]) - 1U] = '\0';
            }

            count++;
            continue; /* Skip the rest of the loop for struct */
        }

        /* Value must be an array with 2 or 3 elements: [data_type, name] or [data_type, name, size] */
        array_size = cJSON_GetArraySize(value_array);
        if (!cJSON_IsArray(value_array) || (array_size < 2) || (array_size > 3)) {
            (void)fprintf(stderr,
                          "Error: '%s' value must be array with 2-3 elements [type, name, size?]\n",
                          matched_key);
            return -1;
        }

        cJSON* data_type_item = cJSON_GetArrayItem(value_array, 0);
        cJSON* name_item = cJSON_GetArrayItem(value_array, 1);

        if (!cJSON_IsString(data_type_item) || !cJSON_IsString(name_item)) {
            (void)fprintf(stderr, "Error: '%s' array elements [0] and [1] must be strings\n",
                          matched_key);
            return -1;
        }

        /* Parse data type */
        args[count].data_type = ParseDataType(data_type_item->valuestring);

        /* Parse argument type from key */
        args[count].arg_type = ParseKernelArgKey(matched_key);

        /* For param, determine scalar type from data_type string */
        if (strcmp(matched_key, "param") == 0) {
            args[count].arg_type = GetScalarArgType(data_type_item->valuestring);
            if (args[count].arg_type == KERNEL_ARG_TYPE_NONE) {
                (void)fprintf(stderr, "Error: Invalid param type: %s\n",
                              data_type_item->valuestring);
                return -1;
            }
        }

        if (args[count].arg_type == KERNEL_ARG_TYPE_NONE) {
            (void)fprintf(stderr, "Error: Invalid kernel arg key: %s\n", matched_key);
            return -1;
        }

        /* Copy source name */
        (void)strncpy(args[count].source_name, name_item->valuestring,
                      sizeof(args[count].source_name) - 1U);
        args[count].source_name[sizeof(args[count].source_name) - 1U] = '\0';

        /* Parse optional buffer size (third element) */
        args[count].buffer_size = 0;
        if (array_size == 3) {
            cJSON* size_item = cJSON_GetArrayItem(value_array, 2);
            if (cJSON_IsNumber(size_item)) {
                args[count].buffer_size = (size_t)size_item->valuedouble;
            } else {
                (void)fprintf(stderr, "Error: '%s' array element [2] must be a number (size)\n",
                              matched_key);
                return -1;
            }
        }

        count++;
    }

    return count;
}

int ParseConfig(const char* filename, Config* config) {
    char* json_str;
    cJSON* root;
    cJSON* item;
    cJSON* kernels;
    cJSON* kernel;
    cJSON* buffers;
    cJSON* buffer;
    cJSON* scalars;
    cJSON* scalar;

    if ((filename == NULL) || (config == NULL)) {
        return -1;
    }

    /* Read JSON file */
    json_str = ReadFileToString(filename);
    if (json_str == NULL) {
        (void)fprintf(stderr, "Error: Failed to open config file: %s\n", filename);
        return -1;
    }

    /* Parse JSON */
    root = cJSON_Parse(json_str);
    /* Note: json_str points to static buffer, no free needed */

    if (root == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            (void)fprintf(stderr, "Error: JSON parse error before: %s\n", error_ptr);
        }
        return -1;
    }

    /* Initialize algorithm-specific fields */
    config->op_id[0] = '\0';
    config->input_image_id[0] = '\0';
    config->output_image_id[0] = '\0';
    config->num_kernels = 0;
    config->custom_buffer_count = 0;
    config->scalar_arg_count = 0;
    config->verification.tolerance = 0.0f;
    config->verification.error_rate_threshold = 0.0f;
    config->verification.golden_source = GOLDEN_SOURCE_C_REF;
    config->verification.golden_file[0] = '\0';

    /* Parse input section */
    item = cJSON_GetObjectItemCaseSensitive(root, "input");
    if (item != NULL) {
        (void)GetJsonString(item, "input_image_id", config->input_image_id,
                            sizeof(config->input_image_id));
    }

    /* Parse output section */
    item = cJSON_GetObjectItemCaseSensitive(root, "output");
    if (item != NULL) {
        (void)GetJsonString(item, "output_image_id", config->output_image_id,
                            sizeof(config->output_image_id));
    }

    /* Parse verification section */
    item = cJSON_GetObjectItemCaseSensitive(root, "verification");
    if (item != NULL) {
        (void)GetJsonFloat(item, "tolerance", &config->verification.tolerance);
        (void)GetJsonFloat(item, "error_rate_threshold",
                           &config->verification.error_rate_threshold);

        cJSON* golden_source = cJSON_GetObjectItemCaseSensitive(item, "golden_source");
        if ((golden_source != NULL) && cJSON_IsString(golden_source)) {
            if (strcmp(golden_source->valuestring, "file") == 0) {
                config->verification.golden_source = GOLDEN_SOURCE_FILE;
            } else {
                config->verification.golden_source = GOLDEN_SOURCE_C_REF;
            }
        }

        (void)GetJsonString(item, "golden_file", config->verification.golden_file,
                            sizeof(config->verification.golden_file));
    }

    /* Parse scalars section */
    scalars = cJSON_GetObjectItemCaseSensitive(root, "scalars");
    if ((scalars != NULL) && cJSON_IsObject(scalars)) {
        cJSON_ArrayForEach(scalar, scalars) {
            /* Skip documentation fields */
            if (scalar->string[0] == '_') {
                continue;
            }

            if (config->scalar_arg_count >= MAX_SCALAR_ARGS) {
                (void)fprintf(stderr, "Error: Too many scalar arguments (max %d)\n",
                              MAX_SCALAR_ARGS);
                cJSON_Delete(root);
                return -1;
            }

            ScalarArgConfig* sc = &config->scalar_args[config->scalar_arg_count];
            (void)memset(sc, 0, sizeof(ScalarArgConfig));

            /* Copy name */
            (void)strncpy(sc->name, scalar->string, sizeof(sc->name) - 1U);
            sc->name[sizeof(sc->name) - 1U] = '\0';

            /* Get type */
            char type_str[32] = "int";
            (void)GetJsonString(scalar, "type", type_str, sizeof(type_str));
            sc->type = ParseScalarTypeStr(type_str);

            if (sc->type == SCALAR_TYPE_NONE) {
                (void)fprintf(stderr, "Error: Invalid scalar type for '%s'\n", sc->name);
                cJSON_Delete(root);
                return -1;
            }

            /* Get value based on type */
            cJSON* value_item = cJSON_GetObjectItemCaseSensitive(scalar, "value");
            if (value_item != NULL) {
                switch (sc->type) {
                    case SCALAR_TYPE_INT:
                        if (cJSON_IsNumber(value_item)) {
                            sc->value.int_value = value_item->valueint;
                        }
                        break;
                    case SCALAR_TYPE_FLOAT:
                        if (cJSON_IsNumber(value_item)) {
                            sc->value.float_value = (float)value_item->valuedouble;
                        }
                        break;
                    case SCALAR_TYPE_SIZE:
                        if (cJSON_IsNumber(value_item)) {
                            sc->value.size_value = (size_t)value_item->valuedouble;
                        } else if (cJSON_IsString(value_item)) {
                            size_t temp;
                            if (EvalExpression(value_item->valuestring, &temp) == 0) {
                                sc->value.size_value = temp;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

            config->scalar_arg_count++;
        }
    }

    /* Parse buffers section */
    buffers = cJSON_GetObjectItemCaseSensitive(root, "buffers");
    if ((buffers != NULL) && cJSON_IsObject(buffers)) {
        cJSON_ArrayForEach(buffer, buffers) {
            /* Skip documentation fields */
            if (buffer->string[0] == '_') {
                continue;
            }

            if (config->custom_buffer_count >= MAX_CUSTOM_BUFFERS) {
                (void)fprintf(stderr, "Error: Too many custom buffers (max %d)\n",
                              MAX_CUSTOM_BUFFERS);
                cJSON_Delete(root);
                return -1;
            }

            CustomBufferConfig* buf = &config->custom_buffers[config->custom_buffer_count];
            (void)memset(buf, 0, sizeof(CustomBufferConfig));

            /* Copy name */
            (void)strncpy(buf->name, buffer->string, sizeof(buf->name) - 1U);
            buf->name[sizeof(buf->name) - 1U] = '\0';

            /* Get type */
            char type_str[32] = "";
            (void)GetJsonString(buffer, "type", type_str, sizeof(type_str));
            buf->type = ParseBufferType(type_str);

            if (buf->type == BUFFER_TYPE_NONE) {
                (void)fprintf(stderr, "Error: Invalid buffer type for '%s': %s\n", buf->name,
                              type_str);
                cJSON_Delete(root);
                return -1;
            }

            /* Get optional fields */
            char data_type_str[32] = "";
            if (GetJsonString(buffer, "data_type", data_type_str, sizeof(data_type_str)) == 0) {
                buf->data_type = ParseDataType(data_type_str);
            }

            (void)GetJsonInt(buffer, "num_elements", &buf->num_elements);
            (void)GetJsonString(buffer, "source_file", buf->source_file, sizeof(buf->source_file));
            (void)GetJsonSize(buffer, "size_bytes", &buf->size_bytes);

            /* Calculate size_bytes for file-backed buffers */
            if ((buf->source_file[0] != '\0') && (buf->size_bytes == 0)) {
                if ((buf->data_type != DATA_TYPE_NONE) && (buf->num_elements > 0)) {
                    buf->size_bytes = GetDataTypeSize(buf->data_type) * (size_t)buf->num_elements;
                }
            }

            config->custom_buffer_count++;
        }
    }

    /* Parse kernels section */
    kernels = cJSON_GetObjectItemCaseSensitive(root, "kernels");
    if ((kernels != NULL) && cJSON_IsObject(kernels)) {
        cJSON_ArrayForEach(kernel, kernels) {
            /* Skip documentation fields */
            if (kernel->string[0] == '_') {
                continue;
            }

            if (config->num_kernels >= MAX_KERNEL_CONFIGS) {
                (void)fprintf(stderr, "Error: Too many kernel configurations (max %d)\n",
                              MAX_KERNEL_CONFIGS);
                cJSON_Delete(root);
                return -1;
            }

            KernelConfig* kc = &config->kernels[config->num_kernels];
            (void)memset(kc, 0, sizeof(KernelConfig));

            /* Copy variant_id (e.g., "v0", "v1") */
            (void)strncpy(kc->variant_id, kernel->string, sizeof(kc->variant_id) - 1U);
            kc->variant_id[sizeof(kc->variant_id) - 1U] = '\0';

            /* Extract variant number (leading digits from variant_id) */
            int variant_num = ExtractVariantNumber(kc->variant_id);
            if (variant_num < 0) {
                (void)fprintf(stderr,
                              "Error: Invalid variant_id format: %s (expected v0, v1, v1f, ...)\n",
                              kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }
            kc->kernel_variant = variant_num;

            /* Get description (optional, defaults to empty) */
            kc->description[0] = '\0';
            (void)GetJsonString(kernel, "description", kc->description, sizeof(kc->description));

            /* Get host_type (default to cl_extension) */
            char host_type_str[32] = "cl_extension";
            (void)GetJsonString(kernel, "host_type", host_type_str, sizeof(host_type_str));
            kc->host_type = ParseHostType(host_type_str);

            /* Get kernel_option (optional user build options, default empty) */
            kc->kernel_option[0] = '\0';
            (void)GetJsonString(kernel, "kernel_option", kc->kernel_option,
                                sizeof(kc->kernel_option));

            /* Get kernel file and function */
            if (GetJsonString(kernel, "kernel_file", kc->kernel_file, sizeof(kc->kernel_file)) !=
                0) {
                (void)fprintf(stderr, "Error: Kernel '%s' missing 'kernel_file'\n", kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }

            if (GetJsonString(kernel, "kernel_function", kc->kernel_function,
                              sizeof(kc->kernel_function)) != 0) {
                (void)fprintf(stderr, "Error: Kernel '%s' missing 'kernel_function'\n",
                              kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }

            /* Get work dimensions */
            if (GetJsonInt(kernel, "work_dim", &kc->work_dim) != 0) {
                (void)fprintf(stderr, "Error: Kernel '%s' missing 'work_dim'\n", kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }

            /* Get global_work_size array */
            cJSON* gws = cJSON_GetObjectItemCaseSensitive(kernel, "global_work_size");
            if ((gws != NULL) && cJSON_IsArray(gws)) {
                int idx = 0;
                cJSON* gws_item;
                cJSON_ArrayForEach(gws_item, gws) {
                    if ((idx < 3) && cJSON_IsNumber(gws_item)) {
                        kc->global_work_size[idx] = (size_t)gws_item->valuedouble;
                        idx++;
                    }
                }
            } else {
                (void)fprintf(stderr, "Error: Kernel '%s' missing 'global_work_size'\n",
                              kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }

            /* Get local_work_size array */
            cJSON* lws = cJSON_GetObjectItemCaseSensitive(kernel, "local_work_size");
            if ((lws != NULL) && cJSON_IsArray(lws)) {
                int idx = 0;
                cJSON* lws_item;
                cJSON_ArrayForEach(lws_item, lws) {
                    if ((idx < 3) && cJSON_IsNumber(lws_item)) {
                        kc->local_work_size[idx] = (size_t)lws_item->valuedouble;
                        idx++;
                    }
                }
            } else {
                (void)fprintf(stderr, "Error: Kernel '%s' missing 'local_work_size'\n",
                              kc->variant_id);
                cJSON_Delete(root);
                return -1;
            }

            /* Parse kernel_args */
            cJSON* args = cJSON_GetObjectItemCaseSensitive(kernel, "kernel_args");
            if (args != NULL) {
                int arg_count = ParseKernelArgsJson(args, kc->kernel_args, MAX_KERNEL_ARGS);
                if (arg_count < 0) {
                    cJSON_Delete(root);
                    return -1;
                }
                kc->kernel_arg_count = arg_count;
            }

            config->num_kernels++;
        }
    }

    cJSON_Delete(root);

    /* Validate custom buffers */
    {
        int i;
        for (i = 0; i < config->custom_buffer_count; i++) {
            CustomBufferConfig* buf = &config->custom_buffers[i];

            /* File-backed buffer: must have source_file, data_type, and num_elements */
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

    // (void)printf("Parsed config file: %s (%d kernels, %d custom buffers, %d scalars)\n", filename,
    //              config->num_kernels, config->custom_buffer_count, config->scalar_arg_count);
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

    /* Treat input as algorithm name - construct config/<name>.json */
    written = snprintf(output, output_size, "config/%s.json", input);
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

int ParseInputsConfig(const char* filename, Config* config) {
    char* json_str;
    cJSON* root;
    cJSON* image;
    int custom_index;

    if ((filename == NULL) || (config == NULL)) {
        return -1;
    }

    /* Read JSON file */
    json_str = ReadFileToString(filename);
    if (json_str == NULL) {
        (void)fprintf(stderr, "Error: Failed to open inputs config file: %s\n", filename);
        return -1;
    }

    /* Parse JSON */
    root = cJSON_Parse(json_str);
    /* Note: json_str points to static buffer, no free needed */

    if (root == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            (void)fprintf(stderr, "Error: JSON parse error before: %s\n", error_ptr);
        }
        return -1;
    }

    /* Initialize input images count */
    config->input_image_count = 0;

    /* First pass: Count image_N entries to determine starting index for custom entries */
    int max_image_index = 0;
    cJSON_ArrayForEach(image, root) {
        if (strncmp(image->string, "image_", 6U) == 0) {
            long temp_long;
            if (SafeStrtol(image->string + 6, &temp_long) && (temp_long >= 1) &&
                (temp_long <= MAX_INPUT_IMAGES)) {
                if ((int)temp_long > max_image_index) {
                    max_image_index = (int)temp_long;
                }
            }
        }
    }

    /* Custom entries start after image_N entries */
    custom_index = max_image_index;

    /* Second pass: Parse all entries */
    cJSON_ArrayForEach(image, root) {
        int image_index;
        InputImageConfig* img;

        /* Handle image_N entries */
        if (strncmp(image->string, "image_", 6U) == 0) {
            long temp_long;
            if (!SafeStrtol(image->string + 6, &temp_long) || (temp_long < 1) ||
                (temp_long > MAX_INPUT_IMAGES)) {
                (void)fprintf(stderr, "Error: Invalid image name: %s\n", image->string);
                cJSON_Delete(root);
                return -1;
            }
            image_index = (int)temp_long - 1;
        }
        /* Handle custom-named entries (e.g., lucas_kanade_input, svd_input) */
        else {
            if (custom_index >= MAX_INPUT_IMAGES) {
                (void)fprintf(stderr, "Error: Too many input images (max %d)\n", MAX_INPUT_IMAGES);
                cJSON_Delete(root);
                return -1;
            }
            image_index = custom_index;
            custom_index++;
        }

        /* Update image count */
        if (image_index + 1 > config->input_image_count) {
            config->input_image_count = image_index + 1;
        }

        img = &config->input_images[image_index];
        (void)memset(img, 0, sizeof(InputImageConfig));

        /* Store entry name for lookup */
        (void)strncpy(img->name, image->string, sizeof(img->name) - 1U);
        img->name[sizeof(img->name) - 1U] = '\0';

        /* Parse image properties - try i_buffer first, then fall back to input */
        if (GetJsonString(image, "i_buffer", img->input_path, sizeof(img->input_path)) != 0) {
            (void)GetJsonString(image, "input", img->input_path, sizeof(img->input_path));
        }
        (void)GetJsonInt(image, "src_width", &img->src_width);
        (void)GetJsonInt(image, "src_height", &img->src_height);
        (void)GetJsonInt(image, "src_channels", &img->src_channels);

        /* src_stride can be a number or expression string */
        size_t stride_val;
        if (GetJsonSize(image, "src_stride", &stride_val) == 0) {
            img->src_stride = (int)stride_val;
        }
    }

    cJSON_Delete(root);

    (void)printf("Parsed inputs config file: %s (%d input images)\n", filename,
                 config->input_image_count);
    return 0;
}

int ParseOutputsConfig(const char* filename, Config* config) {
    char* json_str;
    cJSON* root;
    cJSON* output;
    int custom_index;

    if ((filename == NULL) || (config == NULL)) {
        return -1;
    }

    /* Read JSON file */
    json_str = ReadFileToString(filename);
    if (json_str == NULL) {
        (void)fprintf(stderr, "Error: Failed to open outputs config file: %s\n", filename);
        return -1;
    }

    /* Parse JSON */
    root = cJSON_Parse(json_str);
    /* Note: json_str points to static buffer, no free needed */

    if (root == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            (void)fprintf(stderr, "Error: JSON parse error before: %s\n", error_ptr);
        }
        return -1;
    }

    /* Initialize output images count */
    config->output_image_count = 0;

    /* First pass: Count output_N entries to determine starting index for custom entries */
    int max_output_index = 0;
    cJSON_ArrayForEach(output, root) {
        if (strncmp(output->string, "output_", 7U) == 0) {
            long temp_long;
            if (SafeStrtol(output->string + 7, &temp_long) && (temp_long >= 1) &&
                (temp_long <= MAX_OUTPUT_IMAGES)) {
                if ((int)temp_long > max_output_index) {
                    max_output_index = (int)temp_long;
                }
            }
        }
    }

    /* Custom entries start after output_N entries */
    custom_index = max_output_index;

    /* Second pass: Parse all entries */
    cJSON_ArrayForEach(output, root) {
        int output_index;
        OutputImageConfig* img;

        /* Handle output_N entries */
        if (strncmp(output->string, "output_", 7U) == 0) {
            long temp_long;
            if (!SafeStrtol(output->string + 7, &temp_long) || (temp_long < 1) ||
                (temp_long > MAX_OUTPUT_IMAGES)) {
                (void)fprintf(stderr, "Error: Invalid output name: %s\n", output->string);
                cJSON_Delete(root);
                return -1;
            }
            output_index = (int)temp_long - 1;
        }
        /* Handle custom-named entries (e.g., lucas_kanade_output, svd_output) */
        else {
            if (custom_index >= MAX_OUTPUT_IMAGES) {
                (void)fprintf(stderr, "Error: Too many output images (max %d)\n", MAX_OUTPUT_IMAGES);
                cJSON_Delete(root);
                return -1;
            }
            output_index = custom_index;
            custom_index++;
        }

        /* Update output count */
        if (output_index + 1 > config->output_image_count) {
            config->output_image_count = output_index + 1;
        }

        img = &config->output_images[output_index];
        (void)memset(img, 0, sizeof(OutputImageConfig));

        /* Store entry name for lookup */
        (void)strncpy(img->name, output->string, sizeof(img->name) - 1U);
        img->name[sizeof(img->name) - 1U] = '\0';

        /* Parse output properties - try o_buffer first, then fall back to output */
        if (GetJsonString(output, "o_buffer", img->output_path, sizeof(img->output_path)) != 0) {
            (void)GetJsonString(output, "output", img->output_path, sizeof(img->output_path));
        }
        (void)GetJsonInt(output, "dst_width", &img->dst_width);
        (void)GetJsonInt(output, "dst_height", &img->dst_height);
        (void)GetJsonInt(output, "dst_channels", &img->dst_channels);

        /* dst_stride can be a number or expression string */
        size_t stride_val;
        if (GetJsonSize(output, "dst_stride", &stride_val) == 0) {
            img->dst_stride = (int)stride_val;
        }
    }

    cJSON_Delete(root);

    (void)printf("Parsed outputs config file: %s (%d output images)\n", filename,
                 config->output_image_count);
    return 0;
}
