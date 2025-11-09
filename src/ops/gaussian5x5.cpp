/**
 * @file gaussian5x5.cpp
 * @brief Implementation of Gaussian5x5Op class
 *
 * Contains the implementation of all Gaussian5x5Op methods including
 * test image generation, OpenCL buffer specifications, and result processing.
 *
 * @author OpenCL Study Project
 * @date 2025
 */

#include "gaussian5x5.h"
#include "opRegistry.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

Gaussian5x5Op::Gaussian5x5Op(int width, int height)
    : width_(width), height_(height) {
    inputImage_.resize(width_ * height_);
    outputImage_.resize(width_ * height_);
}

Gaussian5x5Op::~Gaussian5x5Op() {
}

std::string Gaussian5x5Op::getName() const {
    return "Gaussian 5x5 Blur";
}

std::string Gaussian5x5Op::getKernelPath() const {
    return "kernels/gaussian5x5.cl";
}

std::string Gaussian5x5Op::getKernelName() const {
    return "gaussian_blur_5x5";
}

int Gaussian5x5Op::prepareInputData() {
    // Create a test image with noise and patterns
    createTestImage();

    std::cout << "Created test image with noise: " << width_ << "x" << height_ << " pixels" << std::endl;

    // Save input image for comparison
    saveImagePGM("input_gaussian.pgm", inputImage_);

    return 0;
}

BufferSpec Gaussian5x5Op::getInputBufferSpec() {
    // Return specification for input buffer
    // main.cpp will handle allocation and data transfer
    return BufferSpec(inputImage_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_READ_ONLY);
}

BufferSpec Gaussian5x5Op::getOutputBufferSpec() {
    // Return specification for output buffer
    // main.cpp will handle allocation and result retrieval
    return BufferSpec(outputImage_.data(),
                     width_ * height_ * sizeof(unsigned char),
                     CL_MEM_WRITE_ONLY);
}

std::vector<KernelArgument> Gaussian5x5Op::getKernelArguments() {
    // Return kernel argument specifications
    // main.cpp will handle setting these arguments
    std::vector<KernelArgument> args;

    // Kernel signature: gaussian_blur_5x5(global uchar* input, global uchar* output,
    //                                     int width, int height)
    args.push_back(KernelArgument::Buffer(0));                     // input buffer
    args.push_back(KernelArgument::Buffer(1));                     // output buffer
    args.push_back(KernelArgument::Scalar(&width_, sizeof(int)));  // width
    args.push_back(KernelArgument::Scalar(&height_, sizeof(int))); // height

    return args;
}

int Gaussian5x5Op::getGlobalWorkSize(size_t* globalWorkSize) {
    // Use 2D work items, one for each pixel
    globalWorkSize[0] = width_;
    globalWorkSize[1] = height_;

    std::cout << "Global work size: " << globalWorkSize[0] << " x "
              << globalWorkSize[1] << " = " << (width_ * height_) << " work items" << std::endl;

    return 2;  // 2D work dimension
}

int Gaussian5x5Op::verifyResults() {
    // Save output image (results have already been retrieved by main.cpp)
    saveImagePGM("output_gaussian.pgm", outputImage_);

    // TODO: Implement golden image verification
    // For now, just return 0 (no verification)
    std::cout << "Result verification not implemented yet" << std::endl;
    return 0;
}

void Gaussian5x5Op::createTestImage() {
    // Seed random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Create a test image with multiple regions to demonstrate blur:
    // 1. Gradient pattern
    // 2. Random noise overlay
    // 3. Sharp geometric shapes

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            int idx = y * width_ + x;

            // Start with a gradient pattern
            unsigned char baseValue = static_cast<unsigned char>(
                (x * 255 / width_ + y * 255 / height_) / 2
            );

            // Add random noise (±30 intensity)
            int noise = (std::rand() % 61) - 30;
            int noisyValue = baseValue + noise;

            // Clamp to valid range [0, 255]
            if (noisyValue < 0) noisyValue = 0;
            if (noisyValue > 255) noisyValue = 255;

            // Add some geometric shapes (rectangles and circles) with sharp edges
            // This helps visualize edge preservation

            // Rectangle 1: Top-left quadrant
            if (x > width_ / 4 && x < width_ / 2 && y > height_ / 4 && y < height_ / 2) {
                noisyValue = 255;  // Bright rectangle
            }

            // Rectangle 2: Bottom-right quadrant
            if (x > width_ / 2 && x < 3 * width_ / 4 && y > height_ / 2 && y < 3 * height_ / 4) {
                noisyValue = 0;  // Dark rectangle
            }

            // Circle in center
            int cx = width_ / 2;
            int cy = height_ / 2;
            int dx = x - cx;
            int dy = y - cy;
            int distSq = dx * dx + dy * dy;
            int radiusSq = (width_ / 8) * (width_ / 8);
            if (distSq < radiusSq) {
                noisyValue = 128;  // Gray circle
            }

            inputImage_[idx] = static_cast<unsigned char>(noisyValue);
        }
    }
}

void Gaussian5x5Op::saveImagePGM(const char* filename, const std::vector<unsigned char>& image) {
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
    // Static registrar that auto-registers Gaussian5x5Op at program startup
    static OpRegistrar<Gaussian5x5Op> registrar(
        "Gaussian 5x5 Blur",
        []() {
            // Create Gaussian5x5Op with default parameters
            return std::unique_ptr<OpBase>(new Gaussian5x5Op(512, 512));
        }
    );
}
