__kernel void dilate3x3(__global const uchar* input,
                        __global uchar* output,
                        int width,
                        int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    uchar max_val = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int ny = clamp(y + dy, 0, height - 1);
            int nx = clamp(x + dx, 0, width - 1);

            uchar val = input[ny * width + nx];
            max_val = max(max_val, val);
        }
    }

    output[y * width + x] = max_val;
}
