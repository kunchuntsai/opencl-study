#!/usr/bin/env python3
"""
Generate 1D Gaussian kernel data files for gaussian5x5 algorithm.

Creates kernel_x.bin and kernel_y.bin containing the separable 1D Gaussian
weights [1, 4, 6, 4, 1]. The 2D Gaussian kernel is computed as the outer
product of these 1D kernels:

    kernel_2d[i][j] = kernel_y[i] * kernel_x[j]

This results in the normalized 5x5 Gaussian kernel with sum = 256:
    [1,  4,  6,  4,  1]
    [4,  16, 24, 16, 4]
    [6,  24, 36, 24, 6]
    [4,  16, 24, 16, 4]
    [1,  4,  6,  4,  1]
"""
import os
import struct

# 1D Gaussian kernel: [1, 4, 6, 4, 1]
# This is Pascal's triangle row 4, which approximates a Gaussian distribution
kernel = [1.0, 4.0, 6.0, 4.0, 1.0]

# Output directory
script_dir = os.path.dirname(os.path.abspath(__file__))
output_dir = os.path.join(script_dir, '..', 'test_data', 'gaussian5x5')
os.makedirs(output_dir, exist_ok=True)

# Write kernel_x.bin
kernel_x_path = os.path.join(output_dir, 'kernel_x.bin')
with open(kernel_x_path, 'wb') as f:
    for value in kernel:
        f.write(struct.pack('f', value))
print(f"Created {kernel_x_path}")
print(f"  Values: {kernel}")

# Write kernel_y.bin (same as kernel_x for symmetric Gaussian)
kernel_y_path = os.path.join(output_dir, 'kernel_y.bin')
with open(kernel_y_path, 'wb') as f:
    for value in kernel:
        f.write(struct.pack('f', value))
print(f"Created {kernel_y_path}")
print(f"  Values: {kernel}")

# Verify the 2D kernel sum
kernel_sum = sum(kernel[i] * kernel[j] for i in range(5) for j in range(5))
print(f"\n2D Gaussian kernel sum: {kernel_sum}")
print("(Should be 256.0 for the [1,4,6,4,1] kernel)")
