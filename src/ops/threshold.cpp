/**
 * @file threshold.cpp
 * @brief Implementation of ThresholdOp class
 *
 * Contains the implementation of all ThresholdOp methods including
 * image generation, OpenCL buffer management, and result processing.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

#include "threshold.h"
#include "opRegistry.h"
#include <iostream>
#include <fstream>

ThresholdOp::ThresholdOp(int width, int height, unsigned char threshold)
    : width_(width), height_(height), threshold_(threshold) {
    inputImage_.resize(width_ * height_);
    outputImage_.resize(width_ * height_);
}

ThresholdOp::~ThresholdOp() {
}

std::string ThresholdOp::getName() const {
    return "Threshold";
}

std::string ThresholdOp::getKernelPath() const {
    return "kernels/threshold.cl";
}

std::string ThresholdOp::getKernelName() const {
    return "threshold_image";
}

int ThresholdOp::prepareInputData() {
    // Create a test image with gradient pattern
    createTestImage();

    std::cout << "Created test image: " << width_ << "x" << height_ << " pixels" << std::endl;

    // Save input image for comparison
    saveImagePGM("input.pgm", inputImage_);

    return 0;
}

BufferSpec ThresholdOp::getInputBufferSpec() {
    // Return specification for input buffer
    // OpBase will handle allocation and data transfer
    return BufferSpec(inputImage_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_READ_ONLY);
}

BufferSpec ThresholdOp::getOutputBufferSpec() {
    // Return specification for output buffer
    // OpBase will handle allocation and result retrieval
    return BufferSpec(outputImage_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_WRITE_ONLY);
}

std::vector<KernelArgument> ThresholdOp::getKernelArguments() {
    // Return kernel argument specifications
    // OpBase will handle setting these arguments
    std::vector<KernelArgument> args;

    // Kernel signature: threshold_image(global uchar* input, global uchar* output,
    //                                   int width, int height, uchar threshold)
    args.push_back(KernelArgument::Buffer(0));                                  // input buffer
    args.push_back(KernelArgument::Buffer(1));                                  // output buffer
    args.push_back(KernelArgument::Scalar(&width_, sizeof(int)));               // width
    args.push_back(KernelArgument::Scalar(&height_, sizeof(int)));              // height
    args.push_back(KernelArgument::Scalar(&threshold_, sizeof(unsigned char))); // threshold

    return args;
}

int ThresholdOp::getGlobalWorkSize(size_t* globalWorkSize) {
    // Use 2D work items, one for each pixel
    globalWorkSize[0] = width_;
    globalWorkSize[1] = height_;

    std::cout << "Global work size: " << globalWorkSize[0] << " x "
              << globalWorkSize[1] << " = " << (width_ * height_) << " work items" << std::endl;

    return 2;  // 2D work dimension
}

int ThresholdOp::verifyResults() {
    // Save output image (results have already been retrieved by OpBase)
    saveImagePGM("output.pgm", outputImage_);

    // TODO: Implement golden image verification
    // For now, just return 0 (no verification)
    std::cout << "Result verification not implemented yet" << std::endl;
    return 0;
}

void ThresholdOp::createTestImage() {
    // Create a gradient pattern for testing
    // Top-left is dark (0), bottom-right is bright (255)
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            int idx = y * width_ + x;
            // Gradient based on position
            inputImage_[idx] = static_cast<unsigned char>(
                (x * 255 / width_ + y * 255 / height_) / 2
            );
        }
    }
}

void ThresholdOp::saveImagePGM(const char* filename, const std::vector<unsigned char>& image) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to create output file: " << filename << std::endl;
        return;
    }

    // PGM header (P5 = binary grayscale format)
    file << "P5\n" << width_ << " " << height_ << "\n255\n";
    file.write(reinterpret_cast<const char*>(image.data()), image.size());
    file.close();

    std::cout << "Saved image to: " << filename << std::endl;
}

// ============================================================================
// AUTO-REGISTRATION
// ============================================================================
// Register this operation with the global registry at static initialization time.
// This allows the operation to be automatically discovered without modifying main.cpp.
// To add a new operation, just create a similar registrar in your operation's .cpp file.
// ============================================================================

namespace {
    // Static registrar that auto-registers ThresholdOp at program startup
    static OpRegistrar<ThresholdOp> registrar(
        "Threshold",
        []() {
            // Create ThresholdOp with default parameters
            return std::unique_ptr<OpBase>(new ThresholdOp(512, 512, 128));
        }
    );
}
