/**
 * @file cache_manager.c
 * @brief Implementation of cache management for kernel binaries and golden
 * samples
 */

#include "cache_manager.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
#define MAX_KERNEL_BINARY_SIZE (10 * 1024 * 1024) /* 10MB max */
#define MAX_GOLDEN_SAMPLE_SIZE (4096 * 4096)      /* 16MB max */

static unsigned char kernel_binary_buffer[MAX_KERNEL_BINARY_SIZE];
static unsigned char golden_sample_buffer[MAX_GOLDEN_SAMPLE_SIZE];

/* Global storage for current run directory (set by CacheInit) */
static char current_run_dir[MAX_CACHE_PATH] = {0};

/* Helper function to generate timestamp in mmddhhmm format */
static void GetTimestamp(char* buffer, size_t size) {
  time_t now;
  struct tm* timeinfo;

  if ((buffer == NULL) || (size < 9U)) {
    return;
  }

  now = time(NULL);
  timeinfo = localtime(&now);

  if (timeinfo != NULL) {
    (void)snprintf(buffer, size, "%02d%02d%02d%02d", timeinfo->tm_mon + 1, timeinfo->tm_mday,
                   timeinfo->tm_hour, timeinfo->tm_min);
  }
}

/* Helper function to construct algorithm cache directory path with timestamp */
static int BuildAlgorithmDir(const char* algorithm_id, const char* variant_id, char* path,
                             size_t path_size) {
  int result;
  char timestamp[16];

  if ((algorithm_id == NULL) || (variant_id == NULL) || (path == NULL) || (path_size == 0U)) {
    return -1;
  }

  GetTimestamp(timestamp, sizeof(timestamp));
  result = snprintf(path, path_size, "%s/%s_%s_%s", CACHE_BASE_DIR, algorithm_id, variant_id,
                    timestamp);

  if ((result < 0) || ((size_t)result >= path_size)) {
    return -1;
  }

  return 0;
}

/* Helper function to construct kernel cache file path */
static int BuildKernelCachePath(const char* algorithm_id, const char* kernel_name, char* path,
                                   size_t path_size) {
  int result;

  if ((algorithm_id == NULL) || (kernel_name == NULL) || (path == NULL) || (path_size == 0U)) {
    return -1;
  }

  /* Use current run directory if available, otherwise fall back to algorithm_id */
  if (current_run_dir[0] != '\0') {
    result = snprintf(path, path_size, "%s/%s.bin", current_run_dir, kernel_name);
  } else {
    result = snprintf(path, path_size, "%s/%s/%s.bin", CACHE_BASE_DIR, algorithm_id, kernel_name);
  }

  if ((result < 0) || ((size_t)result >= path_size)) {
    return -1;
  }

  return 0;
}

/* Helper function to construct golden sample cache file path */
static int BuildGoldenCachePath(const char* algorithm_id, char* path, size_t path_size) {
  int result;

  if ((algorithm_id == NULL) || (path == NULL) || (path_size == 0U)) {
    return -1;
  }

  /* Use current run directory if available, otherwise fall back to algorithm_id */
  if (current_run_dir[0] != '\0') {
    result = snprintf(path, path_size, "%s/golden.bin", current_run_dir);
  } else {
    result = snprintf(path, path_size, "%s/%s/%s.bin", CACHE_BASE_DIR, algorithm_id, algorithm_id);
  }

  if ((result < 0) || ((size_t)result >= path_size)) {
    return -1;
  }

  return 0;
}

int CacheInit(const char* algorithm_id, const char* variant_id) {
  if ((algorithm_id == NULL) || (variant_id == NULL)) {
    return -1;
  }

  /* Build timestamped run directory path */
  if (BuildAlgorithmDir(algorithm_id, variant_id, current_run_dir, sizeof(current_run_dir)) != 0) {
    return -1;
  }

  /* Create directories - ignore errors if they already exist */
  /* MISRA-C:2023 Rule 17.7: We can ignore the return here as
   * mkdir returns error if directory exists, which is okay */
  (void)mkdir(CACHE_BASE_DIR, 0755);
  (void)mkdir(current_run_dir, 0755);

  (void)printf("Cache directory: %s\n", current_run_dir);

  return 0;
}

const char* CacheGetRunDir(void) {
  if (current_run_dir[0] == '\0') {
    return NULL;
  }
  return current_run_dir;
}

/* ============================================================================
 * KERNEL BINARY CACHING
 * ============================================================================
 */

int CacheKernelExists(const char* algorithm_id, const char* kernel_name) {
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;

  if ((algorithm_id == NULL) || (kernel_name == NULL)) {
    return 0;
  }

  if (BuildKernelCachePath(algorithm_id, kernel_name, cache_path, sizeof(cache_path)) != 0) {
    return 0;
  }

  fp = fopen(cache_path, "rb");
  if (fp == NULL) {
    return 0;
  }

  (void)fclose(fp);
  return 1;
}

int CacheSaveKernelBinary(cl_program program, cl_device_id device, const char* algorithm_id,
                             const char* kernel_name) {
  cl_int err;
  size_t binary_size;
  unsigned char* binary_ptr;
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;
  size_t written;

  if ((program == NULL) || (algorithm_id == NULL) || (kernel_name == NULL)) {
    return -1;
  }

  /* Get binary size */
  err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
  if ((err != CL_SUCCESS) || (binary_size == 0U)) {
    (void)fprintf(stderr, "Error: Failed to get kernel binary size (error: %d)\n", err);
    return -1;
  }

  if (binary_size > MAX_KERNEL_BINARY_SIZE) {
    (void)fprintf(stderr, "Error: Kernel binary too large (%zu bytes, max %u)\n", binary_size,
                  MAX_KERNEL_BINARY_SIZE);
    return -1;
  }

  /* Get binary data */
  binary_ptr = kernel_binary_buffer;
  err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary_ptr, NULL);
  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Error: Failed to get kernel binary (error: %d)\n", err);
    return -1;
  }

  /* Build cache file path */
  if (BuildKernelCachePath(algorithm_id, kernel_name, cache_path, sizeof(cache_path)) != 0) {
    (void)fprintf(stderr, "Error: Failed to build cache path\n");
    return -1;
  }

  /* Save to file */
  fp = fopen(cache_path, "wb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to create kernel cache file: %s\n", cache_path);
    return -1;
  }

  written = fwrite(kernel_binary_buffer, 1U, binary_size, fp);
  if (written != binary_size) {
    (void)fprintf(stderr, "Error: Failed to write kernel binary (%zu of %zu bytes)\n", written,
                  binary_size);
    (void)fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Warning: Failed to close kernel cache file\n");
  }

  (void)printf("Kernel binary cached (%zu bytes): %s\n", binary_size, cache_path);
  return 0;
}

cl_program CacheLoadKernelBinary(cl_context context, cl_device_id device,
                                    const char* algorithm_id, const char* kernel_name) {
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;
  long file_size;
  size_t read_size;
  size_t binary_size;
  const unsigned char* binary_ptr;
  cl_int binary_status;
  cl_int err;
  cl_program program;

  if ((context == NULL) || (algorithm_id == NULL) || (kernel_name == NULL)) {
    return NULL;
  }

  /* Build cache file path */
  if (BuildKernelCachePath(algorithm_id, kernel_name, cache_path, sizeof(cache_path)) != 0) {
    (void)fprintf(stderr, "Error: Failed to build cache path\n");
    return NULL;
  }

  /* Open file */
  fp = fopen(cache_path, "rb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to open kernel cache file: %s\n", cache_path);
    return NULL;
  }

  /* Get file size */
  if (fseek(fp, 0, SEEK_END) != 0) {
    (void)fprintf(stderr, "Error: Failed to seek in cache file\n");
    (void)fclose(fp);
    return NULL;
  }

  file_size = ftell(fp);
  if (file_size < 0) {
    (void)fprintf(stderr, "Error: Failed to get cache file size\n");
    (void)fclose(fp);
    return NULL;
  }

  binary_size = (size_t)file_size;
  if (binary_size > MAX_KERNEL_BINARY_SIZE) {
    (void)fprintf(stderr, "Error: Cached binary too large (%zu bytes)\n", binary_size);
    (void)fclose(fp);
    return NULL;
  }

  if (fseek(fp, 0, SEEK_SET) != 0) {
    (void)fprintf(stderr, "Error: Failed to rewind cache file\n");
    (void)fclose(fp);
    return NULL;
  }

  /* Read binary data */
  read_size = fread(kernel_binary_buffer, 1U, binary_size, fp);
  if (read_size != binary_size) {
    (void)fprintf(stderr, "Error: Failed to read kernel binary (%zu of %zu bytes)\n", read_size,
                  binary_size);
    (void)fclose(fp);
    return NULL;
  }

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Warning: Failed to close cache file\n");
  }

  (void)printf("Loaded cached kernel binary (%zu bytes): %s\n", binary_size, cache_path);

  /* Create program from binary */
  binary_ptr = kernel_binary_buffer;
  program = clCreateProgramWithBinary(context, 1U, &device, &binary_size, &binary_ptr,
                                      &binary_status, &err);
  if ((err != CL_SUCCESS) || (binary_status != CL_SUCCESS)) {
    (void)fprintf(stderr, "Error: Failed to create program from binary (err=%d, status=%d)\n", err,
                  binary_status);
    return NULL;
  }

  /* Build the program (required even for binaries) */
  err = clBuildProgram(program, 1U, &device, NULL, NULL, NULL);
  if (err != CL_SUCCESS) {
    (void)fprintf(stderr, "Error: Failed to build cached binary (error: %d)\n", err);
    (void)clReleaseProgram(program);
    return NULL;
  }

  (void)printf("Kernel binary loaded from cache successfully\n");
  return program;
}

/* ============================================================================
 * GOLDEN SAMPLE CACHING
 * ============================================================================
 */

int CacheGoldenExists(const char* algorithm_id, const char* variant_id) {
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;

  if (algorithm_id == NULL) {
    return 0;
  }

  /* variant_id is ignored - golden samples are per algorithm, not per variant
   */
  (void)variant_id;

  if (BuildGoldenCachePath(algorithm_id, cache_path, sizeof(cache_path)) != 0) {
    return 0;
  }

  fp = fopen(cache_path, "rb");
  if (fp == NULL) {
    return 0;
  }

  (void)fclose(fp);
  return 1;
}

int CacheSaveGolden(const char* algorithm_id, const char* variant_id, const unsigned char* data,
                      size_t size) {
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;
  size_t written;

  if ((algorithm_id == NULL) || (data == NULL) || (size == 0U)) {
    return -1;
  }

  /* variant_id is ignored - golden samples are per algorithm, not per variant
   */
  (void)variant_id;

  if (size > MAX_GOLDEN_SAMPLE_SIZE) {
    (void)fprintf(stderr, "Error: Golden sample too large (%zu bytes, max %u)\n", size,
                  MAX_GOLDEN_SAMPLE_SIZE);
    return -1;
  }

  /* Build cache file path */
  if (BuildGoldenCachePath(algorithm_id, cache_path, sizeof(cache_path)) != 0) {
    (void)fprintf(stderr, "Error: Failed to build cache path\n");
    return -1;
  }

  /* Save to file */
  fp = fopen(cache_path, "wb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to create golden sample file: %s\n", cache_path);
    return -1;
  }

  written = fwrite(data, 1U, size, fp);
  if (written != size) {
    (void)fprintf(stderr, "Error: Failed to write golden sample (%zu of %zu bytes)\n", written,
                  size);
    (void)fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Warning: Failed to close golden sample file\n");
  }

  (void)printf("Golden sample saved (%zu bytes): %s\n", size, cache_path);
  return 0;
}

int CacheLoadGolden(const char* algorithm_id, const char* variant_id, unsigned char* buffer,
                      size_t buffer_size, size_t* actual_size) {
  char cache_path[MAX_CACHE_PATH];
  FILE* fp;
  long file_size;
  size_t read_size;

  if ((algorithm_id == NULL) || (buffer == NULL) || (actual_size == NULL)) {
    return -1;
  }

  /* variant_id is ignored - golden samples are per algorithm, not per variant
   */
  (void)variant_id;

  /* Build cache file path */
  if (BuildGoldenCachePath(algorithm_id, cache_path, sizeof(cache_path)) != 0) {
    (void)fprintf(stderr, "Error: Failed to build cache path\n");
    return -1;
  }

  /* Open file */
  fp = fopen(cache_path, "rb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to open golden sample file: %s\n", cache_path);
    return -1;
  }

  /* Get file size */
  if (fseek(fp, 0, SEEK_END) != 0) {
    (void)fprintf(stderr, "Error: Failed to seek in golden sample file\n");
    (void)fclose(fp);
    return -1;
  }

  file_size = ftell(fp);
  if (file_size < 0) {
    (void)fprintf(stderr, "Error: Failed to get golden sample file size\n");
    (void)fclose(fp);
    return -1;
  }

  if ((size_t)file_size > buffer_size) {
    (void)fprintf(stderr, "Error: Golden sample larger than buffer (%ld > %zu bytes)\n", file_size,
                  buffer_size);
    (void)fclose(fp);
    return -1;
  }

  if (fseek(fp, 0, SEEK_SET) != 0) {
    (void)fprintf(stderr, "Error: Failed to rewind golden sample file\n");
    (void)fclose(fp);
    return -1;
  }

  /* Read data */
  read_size = fread(buffer, 1U, (size_t)file_size, fp);
  if (read_size != (size_t)file_size) {
    (void)fprintf(stderr, "Error: Failed to read golden sample (%zu of %ld bytes)\n", read_size,
                  file_size);
    (void)fclose(fp);
    return -1;
  }

  *actual_size = read_size;

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Warning: Failed to close golden sample file\n");
  }

  (void)printf("Loaded golden sample (%zu bytes): %s\n", read_size, cache_path);
  return 0;
}

int CacheVerifyGolden(const char* algorithm_id, const char* variant_id, const unsigned char* data,
                        size_t size, size_t* differences) {
  size_t golden_size;
  size_t i;
  size_t diff_count;

  if ((algorithm_id == NULL) || (data == NULL) || (differences == NULL)) {
    return -1;
  }

  /* Load golden sample - variant_id can be NULL */
  if (CacheLoadGolden(algorithm_id, variant_id, golden_sample_buffer, MAX_GOLDEN_SAMPLE_SIZE,
                        &golden_size) != 0) {
    return -1;
  }

  /* Check size match */
  if (golden_size != size) {
    (void)fprintf(stderr, "Error: Golden sample size mismatch (expected %zu, got %zu)\n",
                  golden_size, size);
    return -1;
  }

  /* Compare byte by byte */
  diff_count = 0U;
  for (i = 0U; i < size; i++) {
    if (data[i] != golden_sample_buffer[i]) {
      diff_count++;
    }
  }

  *differences = diff_count;

  if (diff_count == 0U) {
    (void)printf("✓ Verification PASSED: Output matches golden sample exactly\n");
    return 1;
  } else {
    (void)fprintf(stderr,
                  "✗ Verification FAILED: %zu byte(s) differ from golden "
                  "sample (%.2f%%)\n",
                  diff_count, ((double)diff_count * 100.0 / (double)size));
    return 0;
  }
}
