#include "utils/verify.h"

#include <math.h>
#include <stdio.h>

#include "utils/safe_ops.h"

int VerifyExactMatch(unsigned char* gpu_output, unsigned char* ref_output, int width, int height,
                     int channels, int tolerance) {
    int errors = 0;
    int total_elements;
    int temp_size;
    int diff;
    int i;
    int actual_channels = (channels > 0) ? channels : 1;

    if ((gpu_output == NULL) || (ref_output == NULL)) {
        return 0;
    }

    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!SafeMulInt(width, height, &temp_size)) {
        (void)fprintf(stderr, "Error: Image dimensions overflow\n");
        return 0;
    }
    if (!SafeMulInt(temp_size, actual_channels, &total_elements)) {
        (void)fprintf(stderr, "Error: Image dimensions overflow (with channels)\n");
        return 0;
    }

    for (i = 0; i < total_elements; i++) {
        diff = (int)gpu_output[i] - (int)ref_output[i];
        if (diff < 0) {
            diff = -diff; /* Absolute value */
        }
        if (diff > tolerance) {
            errors++;
        }
    }

    return (errors == 0) ? 1 : 0; /* Return 1 if passed, 0 if failed */
}

int VerifyWithTolerance(unsigned char* gpu_output, unsigned char* ref_output, int width, int height,
                        int channels, float tolerance, float error_rate_threshold, float* max_error) {
    int errors = 0;
    int total_elements;
    int temp_size;
    float diff;
    float error_rate;
    int i;
    int actual_channels = (channels > 0) ? channels : 1;

    if ((gpu_output == NULL) || (ref_output == NULL) || (max_error == NULL)) {
        return 0;
    }

    *max_error = 0.0f;
    /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
    if (!SafeMulInt(width, height, &temp_size)) {
        (void)fprintf(stderr, "Error: Image dimensions overflow\n");
        return 0;
    }
    if (!SafeMulInt(temp_size, actual_channels, &total_elements)) {
        (void)fprintf(stderr, "Error: Image dimensions overflow (with channels)\n");
        return 0;
    }

    for (i = 0; i < total_elements; i++) {
        diff = fabsf((float)gpu_output[i] - (float)ref_output[i]);
        if (diff > *max_error) {
            *max_error = diff;
        }
        if (diff > tolerance) {
            errors++;
        }
    }

    error_rate = ((float)errors / (float)total_elements);
    return (error_rate <= error_rate_threshold) ? 1 : 0;
}
