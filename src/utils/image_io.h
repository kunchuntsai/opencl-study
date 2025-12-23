/**
 * @file image_io.h
 * @brief Binary image file I/O operations
 *
 * Provides functions for reading and writing raw binary data files.
 * Data is stored as contiguous byte arrays.
 *
 * File format: Raw binary, no header
 *
 * MISRA C 2023 Compliance:
 * - Rule 21.3: Uses static buffers, avoids dynamic allocation
 * - Rule 8.13: Const-correct parameters
 */

#pragma once

#include <stddef.h>

/**
 * @brief Read raw binary data from file
 *
 * Reads a binary file into a static buffer. The file must contain
 * exactly the specified number of bytes.
 *
 * @param[in] filename Path to input file
 * @param[in] size Number of bytes to read
 * @return Pointer to static buffer containing data, or NULL on error
 *
 * @note Returns pointer to static buffer - not thread-safe
 * @note Size must not exceed MAX_IMAGE_SIZE (defined in implementation)
 */
unsigned char* ReadImage(const char* filename, size_t size);

/**
 * @brief Write raw binary data to file
 *
 * Writes data as raw binary to file. Creates or overwrites the
 * specified file with the given number of bytes.
 *
 * @note MISRA C 2023 Rule 8.13: Const qualifier added for read-only data
 *
 * @param[in] filename Path to output file
 * @param[in] data Data to write
 * @param[in] size Number of bytes to write
 * @return 0 on success, -1 on error
 */
int WriteImage(const char* filename, const unsigned char* data, size_t size);
