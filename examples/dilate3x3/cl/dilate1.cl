/**
 * Platform-aware header provides:
 * - PLATFORM_STANDARD / PLATFORM_CL_EXTENSION constants
 * - IS_STANDARD_PLATFORM / IS_CL_EXTENSION macros
 * - Standard implementations (util_mem_copy_uchar, etc.) when HOST_TYPE=0
 * - Empty definitions when HOST_TYPE=1 (custom runtime provides APIs)
 */
#include "platform.h"

__kernel void dilate3x3_optimized(__global const uchar* input,
                                  __global uchar* output,
                                  int width,
                                  int height) {
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);

    // Local memory tile with halo (16x16 work group + 2 pixel border)
    __local uchar tile[18][18];

    // Load center pixel
    if (gx < width && gy < height) {
        tile[ly + 1][lx + 1] = input[gy * width + gx];
    }

    // Load left halo
    if (lx == 0 && gx > 0) {
        tile[ly + 1][0] = input[gy * width + (gx - 1)];
    } else if (lx == 0 && gx == 0) {
        tile[ly + 1][0] = input[gy * width + gx]; // Replicate border
    }

    // Load right halo
    if (lx == get_local_size(0) - 1 && gx < width - 1) {
        tile[ly + 1][lx + 2] = input[gy * width + (gx + 1)];
    } else if (lx == get_local_size(0) - 1 && gx == width - 1) {
        tile[ly + 1][lx + 2] = input[gy * width + gx]; // Replicate border
    }

    // Load top halo
    if (ly == 0 && gy > 0) {
        tile[0][lx + 1] = input[(gy - 1) * width + gx];
    } else if (ly == 0 && gy == 0) {
        tile[0][lx + 1] = input[gy * width + gx]; // Replicate border
    }

    // Load bottom halo
    if (ly == get_local_size(1) - 1 && gy < height - 1) {
        tile[ly + 2][lx + 1] = input[(gy + 1) * width + gx];
    } else if (ly == get_local_size(1) - 1 && gy == height - 1) {
        tile[ly + 2][lx + 1] = input[gy * width + gx]; // Replicate border
    }

    // Load corner halos (top-left)
    if (lx == 0 && ly == 0) {
        int src_y = (gy > 0) ? gy - 1 : gy;
        int src_x = (gx > 0) ? gx - 1 : gx;
        tile[0][0] = input[src_y * width + src_x];
    }

    // Load corner halos (top-right)
    if (lx == get_local_size(0) - 1 && ly == 0) {
        int src_y = (gy > 0) ? gy - 1 : gy;
        int src_x = (gx < width - 1) ? gx + 1 : gx;
        tile[0][lx + 2] = input[src_y * width + src_x];
    }

    // Load corner halos (bottom-left)
    if (lx == 0 && ly == get_local_size(1) - 1) {
        int src_y = (gy < height - 1) ? gy + 1 : gy;
        int src_x = (gx > 0) ? gx - 1 : gx;
        tile[ly + 2][0] = input[src_y * width + src_x];
    }

    // Load corner halos (bottom-right)
    if (lx == get_local_size(0) - 1 && ly == get_local_size(1) - 1) {
        int src_y = (gy < height - 1) ? gy + 1 : gy;
        int src_x = (gx < width - 1) ? gx + 1 : gx;
        tile[ly + 2][lx + 2] = input[src_y * width + src_x];
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (gx >= width || gy >= height) return;

    uchar max_val = 0;
    for (int dy = 0; dy <= 2; dy++) {
        for (int dx = 0; dx <= 2; dx++) {
            uchar val = tile[ly + dy][lx + dx];
            max_val = max(max_val, val);
        }
    }

    output[gy * width + gx] = max_val;
}
