#include "dilate3x3.h"
#include "c_ref/dilate3x3_ref.h"
#include <stdio.h>
#include <math.h>

/* Reference implementation (calls the reference code) */
void dilate3x3_reference(unsigned char* input, unsigned char* output,
                        int width, int height) {
    dilate3x3_ref(input, output, width, height);
}

/* Verification: check if GPU and reference outputs match */
int dilate3x3_verify(unsigned char* gpu_output, unsigned char* ref_output,
                    int width, int height, float* max_error) {
    int errors = 0;
    int total_pixels;
    float diff;
    int i;

    if ((gpu_output == NULL) || (ref_output == NULL) || (max_error == NULL)) {
        return 0;
    }

    *max_error = 0.0f;
    total_pixels = width * height;

    for (i = 0; i < total_pixels; i++) {
        diff = fabsf((float)gpu_output[i] - (float)ref_output[i]);
        if (diff > *max_error) {
            *max_error = diff;
        }
        if (diff > 0.0f) {
            errors++;
        }
    }

    return (errors == 0) ? 1 : 0; /* Return 1 if passed, 0 if failed */
}

void dilate3x3_print_info(void) {
    (void)printf("Dilate 3x3 - Morphological dilation with 3x3 structuring element\n");
}

/* Algorithm registration (called at startup) */
Algorithm dilate3x3_algorithm = {
    .name = "Dilate 3x3",
    .id = "dilate3x3",
    .reference_impl = dilate3x3_reference,
    .verify_result = dilate3x3_verify,
    .print_info = dilate3x3_print_info
};
