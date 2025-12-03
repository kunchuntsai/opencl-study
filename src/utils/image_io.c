#include "image_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "safe_ops.h"

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
/* Using static buffers with maximum size constraints */
#define MAX_IMAGE_SIZE (4096 * 4096) /* Maximum 4K x 4K image */
static unsigned char image_buffer[MAX_IMAGE_SIZE];

unsigned char* read_image(const char* filename, int width, int height) {
  FILE* fp;
  int img_size;
  size_t read_count;
  size_t expected_size;

  if ((filename == NULL) || (width <= 0) || (height <= 0)) {
    (void)fprintf(stderr, "Error: Invalid parameters for read_image\n");
    return NULL;
  }

  /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
  if (!safe_mul_int(width, height, &img_size)) {
    (void)fprintf(stderr, "Error: Image size overflow (width=%d, height=%d)\n",
                  width, height);
    return NULL;
  }

  /* Check if image fits in static buffer */
  if (img_size > MAX_IMAGE_SIZE) {
    (void)fprintf(stderr, "Error: Image too large (%d bytes, max %d)\n",
                  img_size, MAX_IMAGE_SIZE);
    return NULL;
  }

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to open image file: %s\n", filename);
    return NULL;
  }

  expected_size = (size_t)img_size;
  read_count = fread(image_buffer, 1U, expected_size, fp);
  if (read_count != expected_size) {
    (void)fprintf(
        stderr,
        "Error: Failed to read complete image (read %zu of %zu bytes)\n",
        read_count, expected_size);
    (void)fclose(fp);
    return NULL;
  }

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Warning: Failed to close input file: %s\n",
                  filename);
  }

  (void)printf("Read image from %s (%d x %d)\n", filename, width, height);
  return image_buffer;
}

int write_image(const char* filename, const unsigned char* data, int width,
                int height) {
  FILE* fp;
  int img_size;
  size_t write_count;
  size_t expected_size;

  if ((filename == NULL) || (data == NULL) || (width <= 0) || (height <= 0)) {
    (void)fprintf(stderr, "Error: Invalid parameters for write_image\n");
    return -1;
  }

  /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
  if (!safe_mul_int(width, height, &img_size)) {
    (void)fprintf(stderr, "Error: Image size overflow (width=%d, height=%d)\n",
                  width, height);
    return -1;
  }

  fp = fopen(filename, "wb");
  if (fp == NULL) {
    (void)fprintf(stderr, "Error: Failed to create output file: %s\n",
                  filename);
    return -1;
  }

  expected_size = (size_t)img_size;
  write_count = fwrite(data, 1U, expected_size, fp);
  if (write_count != expected_size) {
    (void)fprintf(
        stderr,
        "Error: Failed to write complete image (wrote %zu of %zu bytes)\n",
        write_count, expected_size);
    (void)fclose(fp);
    return -1;
  }

  if (fclose(fp) != 0) {
    (void)fprintf(stderr, "Error: Failed to close output file: %s\n", filename);
    return -1;
  }

  (void)printf("Wrote image to %s (%d x %d)\n", filename, width, height);
  return 0;
}
