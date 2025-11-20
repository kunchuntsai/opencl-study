#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stddef.h>

typedef struct {
    char op_id[32];             // e.g., "dilate3x3"
    char variant_id[32];        // e.g., "v0", "v1"
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
} KernelConfig;

typedef struct {
    char input_image[256];
    char output_image[256];
    int width;
    int height;
    int num_kernels;
    KernelConfig kernels[32];   // Support up to 32 kernel variants
} Config;

// Parse the entire config file
int parse_config(const char* filename, Config* config);

// Get all variants for a specific algorithm
int get_op_variants(Config* config, const char* op_id,
                          KernelConfig** variants, int* count);

#endif // CONFIG_PARSER_H
