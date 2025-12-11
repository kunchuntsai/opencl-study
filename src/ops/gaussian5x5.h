/**
 * @file gaussian5x5.h
 * @brief 5x5 Gaussian blur operation for grayscale images
 *
 * This file defines the Gaussian5x5Op class which implements a 5x5 Gaussian
 * blur filter on grayscale images using OpenCL. Gaussian blur is commonly used
 * for noise reduction and image smoothing in computer vision applications.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

#ifndef GAUSSIAN5X5_H
#define GAUSSIAN5X5_H

#include "opBase.h"
#include <vector>
#include <string>

/**
 * @class Gaussian5x5Op
 * @brief 5x5 Gaussian blur filter operation
 *
 * Implements a Gaussian blur filter using a 5x5 convolution kernel.
 * The operation smooths images by applying weighted averaging, where
 * nearby pixels have more influence than distant pixels.
 *
 * The Gaussian kernel provides a smooth, natural-looking blur that
 * preserves edges better than simple box blur filters.
 *
 * **Use Cases:**
 * - Noise reduction in images
 * - Image preprocessing for computer vision
 * - Creating depth-of-field effects
 * - Anti-aliasing
 *
 * @note Inherits from OpBase which defines the interface
 * @note All OpenCL flow control is handled by main.cpp
 *
 * Example usage:
 * @code
 * Gaussian5x5Op op(512, 512);
 * // main.cpp handles execution using op's specifications
 * // Results saved to input.pgm and output.pgm
 * @endcode
 */
class Gaussian5x5Op : public OpBase {
public:
    /**
     * @brief Construct a Gaussian blur operation
     *
     * Creates a 5x5 Gaussian blur operation with specified image dimensions.
     * The operation will generate a test image internally for demonstration.
     *
     * @param[in] width Image width in pixels (default: 512)
     * @param[in] height Image height in pixels (default: 512)
     *
     * @note Memory for image buffers is pre-allocated based on dimensions
     */
    Gaussian5x5Op(int width = 512, int height = 512);

    /**
     * @brief Destructor
     *
     * Cleans up image data buffers. OpenCL resources are managed
     * by main.cpp.
     */
    virtual ~Gaussian5x5Op();

    // ========================================================================
    // OpBase Interface Implementation
    // ========================================================================

    /**
     * @brief Get operation name
     * @return "Gaussian 5x5 Blur"
     */
    std::string getName() const override;

    /**
     * @brief Get kernel file path
     * @return "kernels/gaussian5x5.cl"
     */
    std::string getKernelPath() const override;

    /**
     * @brief Get kernel function name
     * @return "gaussian_blur_5x5"
     */
    std::string getKernelName() const override;

    /**
     * @brief Prepare input test image
     *
     * Creates a test image with various patterns to demonstrate the blur effect:
     * - High-frequency noise patterns
     * - Sharp edges
     * - Gradients
     *
     * Also saves the input image to "input_gaussian.pgm" for visualization.
     *
     * @return 0 on success, -1 on error
     */
    int prepareInputData() override;

    /**
     * @brief Get input buffer specification
     *
     * Returns specification for input buffer with:
     * - Pointer to inputImage_ data
     * - Size: width * height bytes
     * - Flags: CL_MEM_READ_ONLY
     *
     * @return BufferSpec for input buffer
     */
    BufferSpec getInputBufferSpec() override;

    /**
     * @brief Get output buffer specification
     *
     * Returns specification for output buffer with:
     * - Pointer to outputImage_ data
     * - Size: width * height bytes
     * - Flags: CL_MEM_WRITE_ONLY
     *
     * @return BufferSpec for output buffer
     */
    BufferSpec getOutputBufferSpec() override;

    /**
     * @brief Get kernel arguments specification
     *
     * Returns vector of kernel arguments matching signature:
     * gaussian_blur_5x5(global uchar* input, global uchar* output,
     *                   int width, int height)
     *
     * Arguments:
     * - 0: input buffer
     * - 1: output buffer
     * - 2: width (scalar)
     * - 3: height (scalar)
     *
     * @return Vector of KernelArgument specifications
     */
    std::vector<KernelArgument> getKernelArguments() override;

    /**
     * @brief Get global work size
     *
     * Defines a 2D work grid matching image dimensions.
     * Each work item processes one output pixel.
     *
     * @param[out] globalWorkSize Receives [width, height]
     *
     * @return 2 (2D work dimension)
     */
    int getGlobalWorkSize(size_t* globalWorkSize) override;

    /**
     * @brief Verify results and save output
     *
     * Saves the blurred image to "output_gaussian.pgm".
     * Placeholder for result verification against golden reference.
     *
     * @return 0 (no verification performed)
     * @note Results have already been retrieved by main.cpp
     */
    int verifyResults() override;

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    int width_;                              ///< Image width in pixels
    int height_;                             ///< Image height in pixels

    std::vector<unsigned char> inputImage_;  ///< Host input image buffer
    std::vector<unsigned char> outputImage_; ///< Host output image buffer

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Create test image with noise and patterns
     *
     * Generates a test image in inputImage_ with:
     * - Random noise to demonstrate smoothing
     * - Sharp edges to show edge preservation
     * - Various intensity levels
     *
     * This creates a good test case for evaluating blur quality.
     */
    void createTestImage();

    /**
     * @brief Save image as PGM format
     *
     * Writes image data to a PGM (Portable GrayMap) file which
     * can be viewed with standard image viewers.
     *
     * @param[in] filename Output filename
     * @param[in] image Image data to save
     *
     * @note Uses P5 (binary) PGM format
     * @note Prints error to stderr if file cannot be created
     */
    void saveImagePGM(const char* filename, const std::vector<unsigned char>& image);
};

#endif // GAUSSIAN5X5_H
