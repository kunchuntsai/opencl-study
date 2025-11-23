#pragma once

typedef struct {
    char name[64];              /* Algorithm display name (e.g., "Dilate 3x3") */
    char id[32];                /* Algorithm ID (e.g., "dilate3x3") */

    /* Function pointers */
    void (*reference_impl)(unsigned char* input, unsigned char* output,
                          int width, int height);

    int (*verify_result)(unsigned char* gpu_output, unsigned char* ref_output,
                        int width, int height, float* max_error);

    void (*print_info)(void);   /* Optional: print algorithm-specific info */
} Algorithm;
