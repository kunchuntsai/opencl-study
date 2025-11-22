#pragma once

/* Read raw grayscale image */
unsigned char* read_image(const char* filename, int width, int height);

/* Write raw grayscale image */
/* MISRA-C:2023 Rule 8.13: Added const qualifier for read-only data */
int write_image(const char* filename, const unsigned char* data, int width, int height);
