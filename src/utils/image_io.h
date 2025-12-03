/**
 * @file image_io.h
 * @brief Binary image file I/O operations
 *
 * Provides functions for reading and writing raw grayscale images in
 * binary format. Images are stored as contiguous byte arrays where each
 * byte represents one pixel (0-255).
 *
 * File format: Raw binary, row-major order, no header
 * - Width × Height bytes total
 * - Values: 0 (black) to 255 (white)
 *
 * MISRA C 2023 Compliance:
 * - Rule 21.3: Uses static buffers, avoids dynamic allocation
 * - Rule 1.3: Validates image dimensions to prevent overflow
 * - Rule 8.13: Const-correct parameters
 */

#pragma once

/**
 * @brief Read raw grayscale image from file
 *
 * Reads a binary image file into a static buffer. The file must contain
 * exactly width × height bytes. No header or metadata is expected.
 *
 * @param[in] filename Path to input image file
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 * @return Pointer to static buffer containing image data, or NULL on error
 *
 * @note Returns pointer to static buffer - not thread-safe
 * @note Image size must not exceed MAX_IMAGE_SIZE (defined in implementation)
 */
unsigned char* ReadImage(const char* filename, int width, int height);

/**
 * @brief Write raw grayscale image to file
 *
 * Writes image data as raw binary to file. Creates or overwrites the
 * specified file with width × height bytes.
 *
 * @note MISRA C 2023 Rule 8.13: Const qualifier added for read-only data
 *
 * @param[in] filename Path to output image file
 * @param[in] data Image data to write (width × height bytes)
 * @param[in] width Image width in pixels
 * @param[in] height Image height in pixels
 * @return 0 on success, -1 on error
 */
int WriteImage(const char* filename, const unsigned char* data, int width, int height);
