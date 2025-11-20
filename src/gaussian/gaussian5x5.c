#include "gaussian5x5.h"
#include "c_ref/gaussian5x5_ref.h"
#include <stdio.h>
#include <math.h>

// Reference implementation (calls the reference code)
void gaussian5x5_reference(unsigned char* input, unsigned char* output,
                          int width, int height) {
    gaussian5x5_ref(input, output, width, height);
}

// Verification with tolerance for floating-point differences
int gaussian5x5_verify(unsigned char* gpu_output, unsigned char* ref_output,
                      int width, int height, float* max_error) {
    int errors = 0;
    *max_error = 0.0f;
    float tolerance = 1.0f; // Allow 1 intensity level difference due to rounding

    for (int i = 0; i < width * height; i++) {
        float diff = fabs((float)gpu_output[i] - (float)ref_output[i]);
        if (diff > *max_error) {
            *max_error = diff;
        }
        if (diff > tolerance) {
            errors++;
        }
    }

    float error_rate = (float)errors / (width * height) * 100.0f;
    return (error_rate < 0.1f); // Pass if less than 0.1% pixels differ
}

void gaussian5x5_print_info(void) {
    printf("Gaussian 5x5 - Gaussian blur with 5x5 kernel\n");
}

// Algorithm registration (called at startup)
Algorithm gaussian5x5_algorithm = {
    .name = "Gaussian 5x5",
    .id = "gaussian5x5",
    .reference_impl = gaussian5x5_reference,
    .verify_result = gaussian5x5_verify,
    .print_info = gaussian5x5_print_info
};
