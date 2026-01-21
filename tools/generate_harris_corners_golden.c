/**
 * @file generate_harris_corners_golden.c
 * @brief Standalone tool to generate golden corners output from Harris response
 *
 * This tool reads the Harris response map and applies non-maximum suppression
 * to generate the expected corners output for testing.
 *
 * Usage:
 *   gcc -o generate_harris_corners_golden tools/generate_harris_corners_golden.c -lm
 *   ./generate_harris_corners_golden <response_file> <output_file> <width> <height> <threshold>
 *
 * Example:
 *   ./generate_harris_corners_golden test_data/harris_corner/golden_response.bin \
 *       test_data/harris_corner/golden_corners.bin 1920 1080 10000.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/**
 * @brief Apply non-maximum suppression to Harris response map
 *
 * Matches the harris_nms OpenCL kernel implementation:
 * - Checks if response value exceeds threshold
 * - Checks if it's a local maximum in 3x3 neighborhood
 * - Outputs 255 for corner, 0 otherwise
 *
 * @param response   Input Harris response map (float)
 * @param corners    Output binary corner map (uchar, 255=corner, 0=not corner)
 * @param width      Image width in pixels
 * @param height     Image height in pixels
 * @param threshold  Minimum response value to be considered a corner
 */
void harris_nms_ref(const float* response,
                    unsigned char* corners,
                    int width,
                    int height,
                    float threshold) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;

            /* Skip border pixels */
            if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
                corners[idx] = 0;
                continue;
            }

            float val = response[idx];

            /* Check if below threshold */
            if (val < threshold) {
                corners[idx] = 0;
                continue;
            }

            /* Check if local maximum in 3x3 neighborhood */
            bool is_max = true;
            for (int dy = -1; dy <= 1 && is_max; dy++) {
                for (int dx = -1; dx <= 1 && is_max; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nidx = (y + dy) * width + (x + dx);
                    if (response[nidx] >= val) {
                        is_max = false;
                    }
                }
            }

            corners[idx] = is_max ? 255 : 0;
        }
    }
}

/**
 * @brief Read binary float file
 */
float* read_float_file(const char* filename, size_t num_elements) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    float* data = (float*)malloc(num_elements * sizeof(float));
    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return NULL;
    }

    size_t read_count = fread(data, sizeof(float), num_elements, fp);
    if (read_count != num_elements) {
        fprintf(stderr, "Error: Expected %zu elements, read %zu\n", num_elements, read_count);
        free(data);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return data;
}

/**
 * @brief Write binary uchar file
 */
bool write_uchar_file(const char* filename, const unsigned char* data, size_t num_elements) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return false;
    }

    size_t written = fwrite(data, sizeof(unsigned char), num_elements, fp);
    if (written != num_elements) {
        fprintf(stderr, "Error: Expected to write %zu elements, wrote %zu\n", num_elements, written);
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}

/**
 * @brief Print statistics about corners detected
 */
void print_statistics(const unsigned char* corners, int width, int height) {
    int corner_count = 0;
    int total_pixels = width * height;

    for (int i = 0; i < total_pixels; i++) {
        if (corners[i] == 255) {
            corner_count++;
        }
    }

    printf("Statistics:\n");
    printf("  Total pixels: %d\n", total_pixels);
    printf("  Corners detected: %d\n", corner_count);
    printf("  Corner density: %.4f%%\n", (100.0 * corner_count) / total_pixels);
}

int main(int argc, char** argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <response_file> <output_file> <width> <height> <threshold>\n", argv[0]);
        fprintf(stderr, "Example:\n");
        fprintf(stderr, "  %s test_data/harris_corner/golden_response.bin \\\n", argv[0]);
        fprintf(stderr, "      test_data/harris_corner/golden_corners.bin 1920 1080 10000.0\n");
        return 1;
    }

    const char* response_file = argv[1];
    const char* output_file = argv[2];
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    float threshold = (float)atof(argv[5]);

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: Invalid dimensions %dx%d\n", width, height);
        return 1;
    }

    printf("Generating Harris corners golden data\n");
    printf("  Input: %s\n", response_file);
    printf("  Output: %s\n", output_file);
    printf("  Dimensions: %dx%d\n", width, height);
    printf("  Threshold: %.2f\n", threshold);
    printf("\n");

    size_t num_pixels = (size_t)width * height;

    /* Read Harris response map */
    printf("Reading response map...\n");
    float* response = read_float_file(response_file, num_pixels);
    if (!response) {
        return 1;
    }

    /* Allocate corners output */
    unsigned char* corners = (unsigned char*)malloc(num_pixels);
    if (!corners) {
        fprintf(stderr, "Error: Memory allocation failed for corners\n");
        free(response);
        return 1;
    }

    /* Apply non-maximum suppression */
    printf("Applying non-maximum suppression...\n");
    harris_nms_ref(response, corners, width, height, threshold);

    /* Print statistics */
    print_statistics(corners, width, height);
    printf("\n");

    /* Write output */
    printf("Writing golden corners to %s...\n", output_file);
    if (!write_uchar_file(output_file, corners, num_pixels)) {
        free(response);
        free(corners);
        return 1;
    }

    printf("Done!\n");

    /* Cleanup */
    free(response);
    free(corners);

    return 0;
}
