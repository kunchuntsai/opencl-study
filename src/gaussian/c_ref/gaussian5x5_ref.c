#include "gaussian5x5_ref.h"

void gaussian5x5_ref(unsigned char* input, unsigned char* output,
                     int width, int height) {
    // Gaussian 5x5 kernel (normalized)
    float kernel[5][5] = {
        {1, 4, 6, 4, 1},
        {4, 16, 24, 16, 4},
        {6, 24, 36, 24, 6},
        {4, 16, 24, 16, 4},
        {1, 4, 6, 4, 1}
    };
    float kernel_sum = 256.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;

            // 5x5 window
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;

                    // Clamp to borders
                    ny = (ny < 0) ? 0 : (ny >= height) ? height - 1 : ny;
                    nx = (nx < 0) ? 0 : (nx >= width) ? width - 1 : nx;

                    float weight = kernel[dy + 2][dx + 2];
                    sum += input[ny * width + nx] * weight;
                }
            }

            output[y * width + x] = (unsigned char)(sum / kernel_sum + 0.5f);
        }
    }
}
