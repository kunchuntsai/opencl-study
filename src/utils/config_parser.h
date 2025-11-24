#pragma once

#include <stddef.h>

#define MAX_KERNEL_CONFIGS 32

typedef struct {
    char variant_id[32];        /* e.g., "v0", "v1" */
    char kernel_file[256];
    char kernel_function[64];
    int work_dim;
    size_t global_work_size[3];
    size_t local_work_size[3];
} KernelConfig;

typedef struct {
    char op_id[32];
    char input_image[256];
    char output_image[256];
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
    int num_kernels;
    KernelConfig kernels[MAX_KERNEL_CONFIGS];
} Config;

/* Parse the entire config file */
int parse_config(const char* filename, Config* config);

/* Get all variants for a specific algorithm */
/* MISRA-C:2023: Changed signature to avoid static storage */
int get_op_variants(const Config* config, const char* op_id,
                    KernelConfig* variants[], int* count);
