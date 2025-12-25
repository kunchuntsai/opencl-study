/**
 * @file cache_manager.h
 * @brief Cache management for OpenCL kernel binaries and golden samples
 *
 * This module provides functionality to cache:
 * 1. Compiled OpenCL kernel binaries (to avoid recompilation)
 * 2. Golden sample outputs (for result verification)
 *
 * Cache directory structure:
 * cache/
 *   ├── kernels/     - Compiled kernel binaries (.bin)
 *   └── golden/      - Golden sample outputs (.golden)
 *
 * MISRA-C:2023 Compliance:
 * - Rule 21.3: Avoids dynamic memory allocation
 * - Rule 17.7: All return values are checked
 * - Rule 8.13: Const-correct function parameters
 */

#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <stddef.h>
#include "cl_extension_api.h"

/* Cache base directory (organized per algorithm) */
#define CACHE_BASE_DIR "out"

/* Maximum filename length for cache files */
#define MAX_CACHE_FILENAME 256
#define MAX_CACHE_PATH 512

/* Hash size for source change detection (using FNV-1a 256-bit equivalent via multiple 32-bit) */
#define CACHE_HASH_SIZE 32

/**
 * @brief Initialize cache directory structure for an algorithm
 *
 * Creates the cache directories if they don't exist:
 * - out/{algorithm}_{variant}_{timestamp}/
 *
 * @param algorithm_id Unique identifier for the algorithm (e.g., "dilate3x3")
 * @param variant_id Variant identifier (e.g., "v0", "v1")
 * @return 0 on success, -1 on error
 */
int CacheInit(const char* algorithm_id, const char* variant_id);

/**
 * @brief Get the current run directory path
 *
 * Returns the timestamped directory path for the current run,
 * which was set by CacheInit.
 *
 * @return Pointer to the current run directory path, or NULL if not initialized
 */
const char* CacheGetRunDir(void);

/* ============================================================================
 * KERNEL BINARY CACHING
 * ============================================================================
 */

/**
 * @brief Check if a cached kernel binary exists
 *
 * @param algorithm_id Algorithm identifier (e.g., "dilate3x3")
 * @param kernel_name Name of the kernel (used for cache filename)
 * @return 1 if cached binary exists, 0 otherwise
 */
int CacheKernelExists(const char* algorithm_id, const char* kernel_name);

/**
 * @brief Save compiled kernel binary to cache
 *
 * Extracts the compiled binary from an OpenCL program and saves it to disk.
 * This allows subsequent runs to skip the compilation step.
 *
 * @param program Compiled OpenCL program
 * @param device Device the program was compiled for
 * @param algorithm_id Algorithm identifier (e.g., "dilate3x3")
 * @param kernel_name Name of the kernel (for cache filename)
 * @return 0 on success, -1 on error
 */
int CacheSaveKernelBinary(cl_program program, cl_device_id device, const char* algorithm_id,
                          const char* kernel_name);

/**
 * @brief Load cached kernel binary and create program
 *
 * Loads a previously compiled kernel binary from cache and creates an
 * OpenCL program from it, avoiding recompilation.
 *
 * @param context OpenCL context
 * @param device Device to create program for
 * @param algorithm_id Algorithm identifier (e.g., "dilate3x3")
 * @param kernel_name Name of the kernel (for cache filename)
 * @return OpenCL program object, or NULL on error
 */
cl_program CacheLoadKernelBinary(cl_context context, cl_device_id device, const char* algorithm_id,
                                 const char* kernel_name);

/* ============================================================================
 * SOURCE HASH FOR CACHE INVALIDATION
 * ============================================================================
 */

/**
 * @brief Compute hash of kernel source file for change detection
 *
 * Uses FNV-1a hash algorithm to compute a hash of the source file contents.
 * This hash is used to detect if the source has changed since the binary was cached.
 *
 * @param source_file Path to the .cl source file
 * @param hash_out Buffer to receive hash (must be CACHE_HASH_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int CacheComputeSourceHash(const char* source_file, unsigned char* hash_out);

/**
 * @brief Save source hash alongside kernel binary
 *
 * Saves the source file hash to a .hash file next to the .bin file.
 * This allows detection of source changes on subsequent runs.
 *
 * @param algorithm_id Algorithm identifier
 * @param kernel_name Kernel name (for filename)
 * @param hash Hash data to save (CACHE_HASH_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int CacheSaveSourceHash(const char* algorithm_id, const char* kernel_name,
                        const unsigned char* hash);

/**
 * @brief Load stored source hash from cache
 *
 * @param algorithm_id Algorithm identifier
 * @param kernel_name Kernel name (for filename)
 * @param hash_out Buffer to receive hash (must be CACHE_HASH_SIZE bytes)
 * @return 0 on success, -1 on error (file not found or read error)
 */
int CacheLoadSourceHash(const char* algorithm_id, const char* kernel_name, unsigned char* hash_out);

/**
 * @brief Check if cached kernel is valid (exists AND source unchanged)
 *
 * This function performs a complete validation:
 * 1. Checks if the cached binary file exists
 * 2. Checks if the hash file exists
 * 3. Computes current source hash and compares with stored hash
 *
 * @param algorithm_id Algorithm identifier
 * @param kernel_name Kernel name for cache lookup
 * @param source_file Path to current .cl source file
 * @return 1 if cache is valid and can be used, 0 if invalid/missing/changed
 */
int CacheKernelIsValid(const char* algorithm_id, const char* kernel_name, const char* source_file);


int CacheSaveCustomBinary(CLExtensionContext* ctx);
/* ============================================================================
 * GOLDEN SAMPLE CACHING
 * ============================================================================
 */

/**
 * @brief Check if a golden sample exists
 *
 * @param algorithm_id Unique identifier for the algorithm
 * @param variant_id Variant identifier (can be NULL for c_ref golden samples)
 * @return 1 if golden sample exists, 0 otherwise
 */
int CacheGoldenExists(const char* algorithm_id, const char* variant_id);

/**
 * @brief Save golden sample output to cache
 *
 * Saves the output data as a golden reference for future verification.
 * Golden samples are created from c_ref implementations only.
 *
 * @param algorithm_id Unique identifier for the algorithm
 * @param variant_id Variant identifier (pass NULL for c_ref golden samples)
 * @param data Output data to save
 * @param size Size of data in bytes
 * @return 0 on success, -1 on error
 */
int CacheSaveGolden(const char* algorithm_id, const char* variant_id, const unsigned char* data,
                    size_t size);

/**
 * @brief Load golden sample from cache
 *
 * Loads previously saved golden sample data for verification.
 *
 * @param algorithm_id Unique identifier for the algorithm
 * @param variant_id Variant identifier (pass NULL for c_ref golden samples)
 * @param buffer Buffer to receive the golden sample data
 * @param buffer_size Size of the buffer
 * @param[out] actual_size Actual size of data loaded
 * @return 0 on success, -1 on error
 */
int CacheLoadGolden(const char* algorithm_id, const char* variant_id, unsigned char* buffer,
                    size_t buffer_size, size_t* actual_size);

/**
 * @brief Verify output data against golden sample
 *
 * Compares the current output against the saved golden sample.
 *
 * @param algorithm_id Unique identifier for the algorithm
 * @param variant_id Variant identifier (pass NULL for c_ref golden samples)
 * @param data Current output data
 * @param size Size of data in bytes
 * @param[out] differences Number of byte differences found
 * @return 1 if verification passes (exact match), 0 otherwise, -1 on error
 */
int CacheVerifyGolden(const char* algorithm_id, const char* variant_id, const unsigned char* data,
                      size_t size, size_t* differences);

/**
 * @brief Load golden sample from an external file path
 *
 * Reads a pre-computed golden sample from an arbitrary file path.
 * This is used when golden_source=file is configured, allowing
 * reference data from external sources (e.g., vendor reference outputs).
 *
 * Unlike CacheLoadGolden which uses the cache directory structure,
 * this function reads from the exact path specified.
 *
 * @param golden_file_path Path to the golden sample file
 * @param buffer Buffer to receive the golden sample data
 * @param expected_size Expected size of the golden data in bytes
 * @return 0 on success, -1 on error (file not found, size mismatch, etc.)
 */
int CacheLoadGoldenFromFile(const char* golden_file_path, unsigned char* buffer,
                            size_t expected_size);
