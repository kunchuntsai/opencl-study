#include "dilate3x3_ref.h"

void dilate3x3_ref(unsigned char* input, unsigned char* output,
                   int width, int height) {
    // Handle borders by replication
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char max_val = 0;

            // 3x3 window
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;

                    // Clamp to borders
                    ny = (ny < 0) ? 0 : (ny >= height) ? height - 1 : ny;
                    nx = (nx < 0) ? 0 : (nx >= width) ? width - 1 : nx;

                    unsigned char val = input[ny * width + nx];
                    if (val > max_val) {
                        max_val = val;
                    }
                }
            }

            output[y * width + x] = max_val;
        }
    }
}
