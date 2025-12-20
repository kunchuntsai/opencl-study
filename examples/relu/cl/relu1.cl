/**
 * @file relu1.cl
 * @brief Relu OpenCL kernel with CL extension and custom buffers
 *
 * Kernel arguments (must match kernel_args in config/relu.json):
 * @param input     Input image buffer
 * @param output    Output image buffer
 * @param tmp_buf   Temporary global buffer
 * @param width     Image width in pixels
 * @param height    Image height in pixels
 * @param params    Struct with relu parameters
 */

struct relu_params {
    int relu_v1;
    int relu_v1b;
    float relu_v2;
    float relu_v3;
    int relu_v4;
};

__kernel void relu_optimized(__global const uchar* input,
                             __global uchar* output,
                             __global uchar* tmp_buf,
                             int width,
                             int height,
                             struct relu_params params) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    /* Boundary check */
    if (x >= width || y >= height) return;

    int index = y * width + x;

    /* ReLU: max(0, value) - for uchar, all values are >= 0 */
    /* Using params for threshold-based operation */
    uchar val = input[index];

    /* Apply threshold based on relu_v1 parameter */
    if (val < (uchar)params.relu_v1) {
        val = 0;
    }

    output[index] = val;
}
