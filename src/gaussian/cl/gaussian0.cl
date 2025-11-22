__kernel void gaussian5x5(__global const uchar* input,
                          __global uchar* output,
                          int width,
                          int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    // Gaussian 5x5 kernel weights
    float weights[25] = {
        1, 4, 6, 4, 1,
        4, 16, 24, 16, 4,
        6, 24, 36, 24, 6,
        4, 16, 24, 16, 4,
        1, 4, 6, 4, 1
    };

    float sum = 0.0f;
    int idx = 0;

    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);

            float weight = weights[idx++];
            sum += input[ny * width + nx] * weight;
        }
    }

    output[y * width + x] = convert_uchar_sat(sum / 256.0f);
}
