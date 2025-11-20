#ifndef IMAGE_IO_H
#define IMAGE_IO_H

// Read raw grayscale image
unsigned char* read_image(const char* filename, int width, int height);

// Write raw grayscale image
int write_image(const char* filename, unsigned char* data, int width, int height);

#endif // IMAGE_IO_H
