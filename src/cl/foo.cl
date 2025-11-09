/**
 * @file foo.cl
 * @brief Default template kernel for new OpenCL operations
 *
 * This file provides a simple template kernel that can be used as a starting
 * point for new operations. It demonstrates basic OpenCL kernel structure and
 * serves as the default kernel when OpBase::getKernelPath() is not overridden.
 *
 * When creating a new operation, you can:
 * 1. Copy this file and rename it (e.g., myop.cl)
 * 2. Modify the kernel function for your operation
 * 3. Override getKernelPath() to return your kernel's path
 *
 * @author OpenCL Study Project
 * @date 2025
 */

/**
 * @brief Default pass-through kernel template
 *
 * This is a simple example kernel that copies input data to output without
 * modification. It demonstrates the basic structure of an OpenCL kernel:
 * - 2D work indexing
 * - Parameter passing
 * - Memory access patterns
 * - Bounds checking
 *
 * Use this as a starting template for implementing custom operations.
 *
 * **Operation:** Identity (pass-through)
 * - Simply copies input[idx] to output[idx]
 * - Useful for testing and as a template
 *
 * **Work Organization:**
 * - Work dimension: 2D
 * - Global work size: [width, height]
 * - Each work item processes one data element
 *
 * **Template Customization:**
 * To create a new operation based on this template:
 * 1. Rename the kernel function (e.g., `my_operation`)
 * 2. Modify the algorithm inside the function
 * 3. Add/remove parameters as needed
 * 4. Update the host code to match parameter changes
 *
 * @param[in]  input  Input data buffer (read-only)
 * @param[out] output Output data buffer (write-only)
 * @param[in]  width  Data width (first dimension)
 * @param[in]  height Data height (second dimension)
 *
 * @note This is a minimal template - add parameters as needed
 * @note Default implementation is a simple pass-through (identity operation)
 *
 * Example usage:
 * @code
 * // Host-side kernel argument setup
 * clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
 * clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
 * clSetKernelArg(kernel, 2, sizeof(int), &width);
 * clSetKernelArg(kernel, 3, sizeof(int), &height);
 *
 * // Launch kernel
 * size_t globalWorkSize[2] = {width, height};
 * clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
 * @endcode
 */
__kernel void foo(
    __global const unsigned char* input,   // Input data (read-only)
    __global unsigned char* output,        // Output data (write-only)
    const int width,                       // Data width
    const int height                       // Data height
)
{
    // Get the global position in the 2D grid
    // Each work item processes one element
    int x = get_global_id(0);  // Column index
    int y = get_global_id(1);  // Row index

    // Bounds check: ensure we don't access out-of-range memory
    // This is important if global work size doesn't exactly match data dimensions
    if (x >= width || y >= height) {
        return;
    }

    // Calculate the linear index in the flattened array
    // For 2D data stored row-by-row: index = y * width + x
    int idx = y * width + x;

    // Simple pass-through operation (copy input to output)
    // Replace this with your custom operation logic
    output[idx] = input[idx];
}
