#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper function to trim whitespace
static char* trim(char* str) {
    char* end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

// Parse a size_t array from a string like "1024,1024" or "1024"
static int parse_size_array(const char* str, size_t* arr, int max_count) {
    char buffer[256];
    strncpy(buffer, str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    int count = 0;
    char* token = strtok(buffer, ",");
    while (token != NULL && count < max_count) {
        arr[count++] = (size_t)atoi(trim(token));
        token = strtok(NULL, ",");
    }

    return count;
}

int parse_config(const char* filename, Config* config) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open config file: %s\n", filename);
        return -1;
    }

    char line[512];
    char section[64] = "";
    int kernel_index = -1;

    // Initialize config
    memset(config, 0, sizeof(Config));
    config->num_kernels = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim(line);

        // Skip empty lines and comments
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        // Check for section header
        if (trimmed[0] == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';

                // If section starts with "kernel.", it's a kernel configuration
                if (strncmp(section, "kernel.", 7) == 0) {
                    kernel_index = config->num_kernels++;
                    if (config->num_kernels > 32) {
                        fprintf(stderr, "Error: Too many kernel configurations (max 32)\n");
                        fclose(fp);
                        return -1;
                    }
                }
            }
            continue;
        }

        // Parse key=value pairs
        char* equals = strchr(trimmed, '=');
        if (!equals) continue;

        *equals = '\0';
        char* key = trim(trimmed);
        char* value = trim(equals + 1);

        // Parse based on section
        if (strcmp(section, "image") == 0) {
            if (strcmp(key, "input") == 0) {
                strncpy(config->input_image, value, sizeof(config->input_image) - 1);
            } else if (strcmp(key, "output") == 0) {
                strncpy(config->output_image, value, sizeof(config->output_image) - 1);
            } else if (strcmp(key, "width") == 0) {
                config->width = atoi(value);
            } else if (strcmp(key, "height") == 0) {
                config->height = atoi(value);
            }
        } else if (strncmp(section, "kernel.", 7) == 0 && kernel_index >= 0) {
            KernelConfig* kc = &config->kernels[kernel_index];

            if (strcmp(key, "op_id") == 0) {
                strncpy(kc->op_id, value, sizeof(kc->op_id) - 1);
            } else if (strcmp(key, "variant_id") == 0) {
                strncpy(kc->variant_id, value, sizeof(kc->variant_id) - 1);
            } else if (strcmp(key, "kernel_file") == 0) {
                strncpy(kc->kernel_file, value, sizeof(kc->kernel_file) - 1);
            } else if (strcmp(key, "kernel_function") == 0) {
                strncpy(kc->kernel_function, value, sizeof(kc->kernel_function) - 1);
            } else if (strcmp(key, "work_dim") == 0) {
                kc->work_dim = atoi(value);
            } else if (strcmp(key, "global_work_size") == 0) {
                parse_size_array(value, kc->global_work_size, 3);
            } else if (strcmp(key, "local_work_size") == 0) {
                parse_size_array(value, kc->local_work_size, 3);
            }
        }
    }

    fclose(fp);
    printf("Parsed config file: %s (%d kernel configurations)\n",
           filename, config->num_kernels);
    return 0;
}

int get_op_variants(Config* config, const char* op_id,
                          KernelConfig** variants, int* count) {
    static KernelConfig* variant_ptrs[32];
    int variant_count = 0;

    for (int i = 0; i < config->num_kernels; i++) {
        if (strcmp(config->kernels[i].op_id, op_id) == 0) {
            variant_ptrs[variant_count++] = &config->kernels[i];
        }
    }

    *variants = variant_ptrs[0];
    *count = variant_count;
    return variant_count;
}
