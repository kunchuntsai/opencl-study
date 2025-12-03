#include <stddef.h>

#include "../../utils/op_interface.h"
#include "../../utils/op_registry.h"
#include "../../utils/safe_ops.h"
#include "../../utils/verify.h"

/* Helper function to clamp coordinates to image bounds */
static int clamp_coord(int coord, int max_coord) {
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
static unsigned char get_pixel_safe(const unsigned char* input, int x, int y,
                                    int width, int height) {
  int clamped_x;
  int clamped_y;
  int index;

  if ((input == NULL) || (width <= 0) || (height <= 0)) {
    return 0U;
  }

  clamped_x = clamp_coord(x, width);
  clamped_y = clamp_coord(y, height);
  index = clamped_y * width + clamped_x;

  return input[index];
}

void dilate3x3_ref(const OpParams* params) {
  int y;
  int x;
  int dy;
  int dx;
  int ny;
  int nx;
  unsigned char max_val;
  unsigned char val;
  int output_index;
  int total_pixels;
  int width;
  int height;
  unsigned char* input;
  unsigned char* output;

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

  /* MISRA-C:2023 Rule 1.3: Check for integer overflow */
  if (!safe_mul_int(width, height, &total_pixels)) {
    return; /* Overflow detected */
  }

  /* Handle borders by replication */
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      max_val = 0U;

      /* 3x3 window */
      for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
          ny = y + dy;
          nx = x + dx;

          /* Get pixel value with bounds checking */
          val = get_pixel_safe(input, nx, ny, width, height);
          if (val > max_val) {
            max_val = val;
          }
        }
      }

      output_index = y * width + x;

      /* MISRA-C:2023 Rule 18.1: Bounds check before write */
      if (output_index < total_pixels) {
        output[output_index] = max_val;
      }
    }
  }
}

/* Verification: check if GPU and reference outputs match exactly */
int dilate3x3_verify(const OpParams* params, float* max_error) {
  int result;

  if (params == NULL) {
    return 0;
  }

  /* Use tolerance of 0 for exact match */
  result = verify_exact_match(params->gpu_output, params->ref_output,
                              params->dst_width, params->dst_height, 0);

  /* Set max_error to 0 if verification passed */
  if (max_error != NULL) {
    *max_error = (result == 1) ? 0.0f : 1.0f;
  }

  return result;
}

/* Kernel argument setter - sets 4 standard arguments */
int dilate3x3_set_kernel_args(cl_kernel kernel, cl_mem input_buf,
                              cl_mem output_buf, const OpParams* params) {
  cl_uint arg_idx = 0U;

  if ((kernel == NULL) || (params == NULL)) {
    return -1;
  }

  /* Set all 4 kernel arguments */
  if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &input_buf) !=
      CL_SUCCESS) {
    return -1;
  }
  if (clSetKernelArg(kernel, arg_idx++, sizeof(cl_mem), &output_buf) !=
      CL_SUCCESS) {
    return -1;
  }
  if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_width) !=
      CL_SUCCESS) {
    return -1;
  }
  if (clSetKernelArg(kernel, arg_idx++, sizeof(int), &params->src_height) !=
      CL_SUCCESS) {
    return -1;
  }

  return 0;
}
