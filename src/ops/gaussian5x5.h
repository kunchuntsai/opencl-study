/**
 * @file gaussian5x5.h
 * @brief 5x5 Gaussian blur operation for grayscale images
 */

#ifndef GAUSSIAN5X5_H
#define GAUSSIAN5X5_H

#include "opBase.h"
#include <vector>
#include <string>

/**
 * @class Gaussian5x5Op
 * @brief 5x5 Gaussian blur using weighted convolution for image smoothing
 */
class Gaussian5x5Op : public OpBase {
public:
    /**
     * @brief Construct Gaussian blur operation
     * @param width Image width (default: 512)
     * @param height Image height (default: 512)
     */
    Gaussian5x5Op(int width = 512, int height = 512);

    /** @brief Destructor */
    virtual ~Gaussian5x5Op();

    std::string getName() const override;
    std::string getKernelPath() const override;
    std::string getKernelName() const override;

    /** @brief Create test image with noise and patterns, save to input_gaussian.pgm */
    int prepareInputData() override;

    /** @brief Get input buffer specification */
    BufferSpec getInputBufferSpec() override;

    /** @brief Get output buffer specification */
    BufferSpec getOutputBufferSpec() override;

    /** @brief Get kernel arguments: input, output, width, height */
    std::vector<KernelArgument> getKernelArguments() override;

    /** @brief Get global work size (2D: width x height) */
    int getGlobalWorkSize(size_t* globalWorkSize) override;

    /** @brief Save blurred image to output_gaussian.pgm */
    int verifyResults() override;

private:
    int width_;                              ///< Image width
    int height_;                             ///< Image height
    std::vector<unsigned char> inputImage_;  ///< Input buffer
    std::vector<unsigned char> outputImage_; ///< Output buffer

    /** @brief Generate test image with noise, edges, and patterns */
    void createTestImage();

    /** @brief Save image as PGM format */
    void saveImagePGM(const char* filename, const std::vector<unsigned char>& image);
};

#endif // GAUSSIAN5X5_H
