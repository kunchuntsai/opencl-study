#include "config_parser.h"
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

int parse_config(const char* filename, Config* config) {
    FILE* fp;
    char line[MAX_LINE_LENGTH];
    char section[MAX_SECTION_LENGTH] = "";
    int kernel_index = -1;
    char* trimmed;
    char* equals;
    char* key;
    char* value;
    char* end;
    long temp_long;

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
                    config->num_kernels++;
                    if (config->num_kernels > MAX_KERNEL_CONFIGS) {
                        (void)fprintf(stderr, "Error: Too many kernel configurations (max %d)\n",
                                      MAX_KERNEL_CONFIGS);
                        (void)fclose(fp);
                        return -1;
                    }
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

        /* Parse based on section */
        if (strcmp(section, "image") == 0) {
            if (strcmp(key, "input") == 0) {
                (void)strncpy(config->input_image, value, sizeof(config->input_image) - 1U);
                config->input_image[sizeof(config->input_image) - 1U] = '\0';
            } else if (strcmp(key, "output") == 0) {
                (void)strncpy(config->output_image, value, sizeof(config->output_image) - 1U);
                config->output_image[sizeof(config->output_image) - 1U] = '\0';
            } else if (strcmp(key, "width") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->width = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid width value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else if (strcmp(key, "height") == 0) {
                if (safe_strtol(value, &temp_long) && (temp_long > 0)) {
                    config->height = (int)temp_long;
                } else {
                    (void)fprintf(stderr, "Error: Invalid height value: %s\n", value);
                    (void)fclose(fp);
                    return -1;
                }
            } else {
                /* Unknown key in image section */
            }
        } else if ((strncmp(section, "kernel.", 7U) == 0) && (kernel_index >= 0)) {
            KernelConfig* kc = &config->kernels[kernel_index];

            if (strcmp(key, "op_id") == 0) {
                (void)strncpy(kc->op_id, value, sizeof(kc->op_id) - 1U);
                kc->op_id[sizeof(kc->op_id) - 1U] = '\0';
            } else if (strcmp(key, "variant_id") == 0) {
                (void)strncpy(kc->variant_id, value, sizeof(kc->variant_id) - 1U);
                kc->variant_id[sizeof(kc->variant_id) - 1U] = '\0';
            } else if (strcmp(key, "kernel_file") == 0) {
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
            } else {
                /* Unknown key in kernel section */
            }
        } else {
            /* Unknown section or invalid kernel index */
        }
    }

    (void)fclose(fp);
    (void)printf("Parsed config file: %s (%d kernel configurations)\n",
                 filename, config->num_kernels);
    return 0;
}

/* MISRA-C:2023 Rule 8.14: Removed static variable for reentrancy */
int get_op_variants(const Config* config, const char* op_id,
                    KernelConfig* variants[], int* count) {
    int variant_count = 0;
    int i;

    if ((config == NULL) || (op_id == NULL) || (variants == NULL) || (count == NULL)) {
        return -1;
    }

    for (i = 0; i < config->num_kernels; i++) {
        if (strcmp(config->kernels[i].op_id, op_id) == 0) {
            if (variant_count < MAX_KERNEL_CONFIGS) {
                /* Casting away const for backwards compatibility */
                /* In a stricter implementation, Config should not be const */
                /* or variants should be const KernelConfig* */
                variants[variant_count] = (KernelConfig*)&config->kernels[i];
                variant_count++;
            }
        }
    }

    *count = variant_count;
    return 0;
}
