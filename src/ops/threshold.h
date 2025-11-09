/**
 * @file threshold.h
 * @brief Binary threshold operation for grayscale images
 *
 * This file defines the ThresholdOp class which implements a simple
 * binary thresholding operation on grayscale images using OpenCL.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

#ifndef THRESHOLD_H
#define THRESHOLD_H

#include "opBase.h"
#include <vector>
#include <string>

/**
 * @class ThresholdOp
 * @brief Binary threshold operation for image processing
 *
 * Implements binary thresholding where pixels with values >= threshold
 * become white (255) and pixels < threshold become black (0).
 *
 * This operation demonstrates a simple but complete OpenCL workflow:
 * - Generates a gradient test image
 * - Transfers data to GPU
 * - Applies threshold operation in parallel
 * - Retrieves and saves results
 *
 * @note Inherits from OpBase which handles the OpenCL orchestration
 *
 * Example usage:
 * @code
 * ThresholdOp op(512, 512, 128);  // 512x512 image, threshold=128
 * op.execute(context, queue, device);
 * // Results saved to input.pgm and output.pgm
 * @endcode
 */
class ThresholdOp : public OpBase {
public:
    /**
     * @brief Construct a threshold operation
     *
     * Creates a threshold operation with specified image dimensions
     * and threshold value. The operation will generate a gradient
     * test image internally.
     *
     * @param[in] width Image width in pixels (default: 512)
     * @param[in] height Image height in pixels (default: 512)
     * @param[in] threshold Threshold value 0-255 (default: 128)
     *                      Pixels >= threshold become white (255)
     *                      Pixels < threshold become black (0)
     *
     * @note Memory for image buffers is pre-allocated based on dimensions
     */
    ThresholdOp(int width = 512, int height = 512, unsigned char threshold = 128);

    /**
     * @brief Destructor
     *
     * Cleans up image data buffers. OpenCL resources are managed
     * by the base class.
     */
    virtual ~ThresholdOp();

    // ========================================================================
    // OpBase Interface Implementation
    // ========================================================================

    /**
     * @brief Get operation name
     * @return "Threshold"
     */
    std::string getName() const override;

    /**
     * @brief Get kernel file path
     * @return "kernels/threshold.cl"
     */
    std::string getKernelPath() const override;

    /**
     * @brief Get kernel function name
     * @return "threshold_image"
     */
    std::string getKernelName() const override;

    /**
     * @brief Prepare input test image
     *
     * Creates a gradient test image where:
     * - Top-left corner is dark (value 0)
     * - Bottom-right corner is bright (value 255)
     * - Gradient interpolated in between
     *
     * Also saves the input image to "input.pgm" for visualization.
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
     * threshold_image(global uchar* input, global uchar* output,
     *                int width, int height, uchar threshold)
     *
     * Arguments:
     * - 0: input buffer
     * - 1: output buffer
     * - 2: width (scalar)
     * - 3: height (scalar)
     * - 4: threshold value (scalar)
     *
     * @return Vector of KernelArgument specifications
     */
    std::vector<KernelArgument> getKernelArguments() override;

    /**
     * @brief Get global work size
     *
     * Defines a 2D work grid matching image dimensions.
     * Each work item processes one pixel.
     *
     * @param[out] globalWorkSize Receives [width, height]
     *
     * @return 2 (2D work dimension)
     */
    int getGlobalWorkSize(size_t* globalWorkSize) override;

    /**
     * @brief Verify results and save output
     *
     * Saves the thresholded image to "output.pgm".
     * Placeholder for result verification against golden reference.
     *
     * @return 0 (no verification performed)
     * @note Results have already been retrieved by OpBase
     */
    int verifyResults() override;

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    int width_;                              ///< Image width in pixels
    int height_;                             ///< Image height in pixels
    unsigned char threshold_;                ///< Threshold value (0-255)

    std::vector<unsigned char> inputImage_;  ///< Host input image buffer
    std::vector<unsigned char> outputImage_; ///< Host output image buffer

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Create gradient test image
     *
     * Generates a grayscale gradient image in inputImage_ where
     * pixel value increases from top-left (0) to bottom-right (255).
     *
     * Formula: pixel[x,y] = (x*255/width + y*255/height) / 2
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

#endif // THRESHOLD_H
