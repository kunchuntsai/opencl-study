#include "image_io.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char* read_image(const char* filename, int width, int height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open image file: %s\n", filename);
        return NULL;
    }

    int img_size = width * height;
    unsigned char* data = (unsigned char*)malloc(img_size);
    if (!data) {
        fprintf(stderr, "Error: Failed to allocate memory for image\n");
        fclose(fp);
        return NULL;
    }

    size_t read_count = fread(data, 1, img_size, fp);
    if (read_count != img_size) {
        fprintf(stderr, "Error: Failed to read complete image (read %zu of %d bytes)\n",
                read_count, img_size);
        free(data);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    printf("Read image from %s (%d x %d)\n", filename, width, height);
    return data;
}

int write_image(const char* filename, unsigned char* data, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to create output file: %s\n", filename);
        return -1;
    }

    int img_size = width * height;
    size_t write_count = fwrite(data, 1, img_size, fp);
    if (write_count != img_size) {
        fprintf(stderr, "Error: Failed to write complete image (wrote %zu of %d bytes)\n",
                write_count, img_size);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("Wrote image to %s (%d x %d)\n", filename, width, height);
    return 0;
}
