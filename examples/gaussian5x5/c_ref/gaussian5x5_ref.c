#include <stddef.h>
#include <stdio.h>

#include "op_interface.h"
#include "op_registry.h"
#include "utils/safe_ops.h"

/* Helper function to clamp coordinates to image bounds */
static int ClampCoord(int coord, int max_coord) {
  int result;

  if (coord < 0) {
    result = 0;
  } else if (coord >= max_coord) {
    result = max_coord - 1;
  } else {
    result = coord;
  }

  return result;
}

/* MISRA-C:2023 Rule 18.1: Add bounds checking for array access */
static int GetPixelSafe(const unsigned char* input, int x, int y, int width, int height) {
  int clamped_x;
  int clamped_y;
  int index;

  if ((input == NULL) || (width <= 0) || (height <= 0)) {
    return 0;
  }

  clamped_x = ClampCoord(x, width);
  clamped_y = ClampCoord(y, height);
  index = clamped_y * width + clamped_x;

  return (int)input[index];
}

void Gaussian5x5Ref(const OpParams* params) {
  /* Separable Gaussian 5x5 using 1D kernels from custom buffers */
  /* This matches the OpenCL implementation which uses kernel_x and kernel_y */
  int y;
  int x;
  int dy;
  int dx;
  int ny;
  int nx;
  float sum;
  float kernel_sum;
  float weight;
  int pixel_val;
  int output_index;
  int total_pixels;
  unsigned char result;
  int width;
  int height;
  unsigned char* input;
  unsigned char* output;
  CustomBuffers* custom_buffers;
  const float* kernel_x;
  const float* kernel_y;

  if (params == NULL) {
    return;
  }

  /* Extract parameters */
  input = params->input;
  output = params->output;
  width = params->src_width;
  height = params->src_height;

  if ((input == NULL) || (output == NULL) || (width <= 0) || (height <= 0)) {
    return;
  }

  /* Get 1D kernels from custom buffers */
  if (params->custom_buffers == NULL) {
    (void)fprintf(stderr, "Error: Gaussian reference requires custom buffers for kernel data\n");
    return;
  }
  custom_buffers = params->custom_buffers;

  /* Determine kernel buffer indices based on buffer configuration */
  /* Buffers defined in config: tmp_global(0), tmp_global2(1), kernel_x(2), kernel_y(3) */
  /* All variants use the same buffer layout */
  int kernel_x_idx = 2;
  int kernel_y_idx = 3;
  int min_buffers = kernel_y_idx + 1; /* Need at least up to kernel_y index */

  if (custom_buffers->count < min_buffers) {
    (void)fprintf(stderr,
                  "Error: Gaussian reference requires at least %d custom "
                  "buffers (got %d)\n",
                  min_buffers, custom_buffers->count);
    return;
  }

  /* Get kernel_x and kernel_y from appropriate buffer indices */
  /* (tmp_buffer(s) are only used by GPU, not by reference implementation) */
  kernel_x = (const float*)custom_buffers->buffers[kernel_x_idx].host_data;
  kernel_y = (const float*)custom_buffers->buffers[kernel_y_idx].host_data;

  if ((kernel_x == NULL) || (kernel_y == NULL)) {
    (void)fprintf(stderr, "Error: Gaussian kernel data not loaded\n");
    return;
  }

  /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
  if (!SafeMulInt(width, height, &total_pixels)) {
    return; /* Overflow detected */
  }

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      sum = 0.0f;
      kernel_sum = 0.0f;

      /* 5x5 window with separable convolution */
      /* 2D weight = kernel_y[i] * kernel_x[j] */
      for (dy = -2; dy <= 2; dy++) {
        for (dx = -2; dx <= 2; dx++) {
          ny = y + dy;
          nx = x + dx;

          /* Get pixel value with bounds checking */
          pixel_val = GetPixelSafe(input, nx, ny, width, height);

          /* Compute 2D weight as outer product of 1D kernels */
          weight = kernel_y[dy + 2] * kernel_x[dx + 2];
          kernel_sum += weight;
          sum += (float)pixel_val * weight;
        }
      }

      /* Convert to unsigned char with rounding */
      result = (unsigned char)((sum / kernel_sum) + 0.5f);
      output_index = y * width + x;

      /* MISRA-C:2023 Rule 18.1: Bounds check before write */
      if (output_index < total_pixels) {
        output[output_index] = result;
      }
    }
  }
}

/*
 * NOTE: Registration of this algorithm happens in auto_registry.c
 * See src/utils/auto_registry.c for the registration code.
 *
 * Verification is now config-driven via [verification] section in .ini files.
 * Kernel arguments are config-driven via kernel_args in [kernel.*] sections.
 */
