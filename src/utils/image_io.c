#include "image_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MISRA-C:2023 Rule 21.3: Avoid dynamic memory allocation */
/* Using static buffers with maximum size constraints */
#define MAX_IMAGE_SIZE (4096 * 4096) /* Maximum 16MB buffer */
static unsigned char image_buffer[MAX_IMAGE_SIZE];

unsigned char* ReadImage(const char* filename, size_t size) {
    FILE* fp;
    size_t read_count;

    if ((filename == NULL) || (size == 0U)) {
        (void)fprintf(stderr, "Error: Invalid parameters for ReadImage\n");
        return NULL;
    }

    /* Check if data fits in static buffer */
    if (size > MAX_IMAGE_SIZE) {
        (void)fprintf(stderr, "Error: Data too large (%zu bytes, max %d)\n", size,
                      MAX_IMAGE_SIZE);
        return NULL;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        (void)fprintf(stderr, "Error: Failed to open file: %s\n", filename);
        return NULL;
    }

    read_count = fread(image_buffer, 1U, size, fp);
    if (read_count != size) {
        (void)fprintf(stderr, "Error: Failed to read complete data (read %zu of %zu bytes)\n",
                      read_count, size);
        (void)fclose(fp);
        return NULL;
    }

    if (fclose(fp) != 0) {
        (void)fprintf(stderr, "Warning: Failed to close input file: %s\n", filename);
    }

    (void)printf("Read %zu bytes from %s\n", size, filename);
    return image_buffer;
}

int WriteImage(const char* filename, const unsigned char* data, size_t size) {
    FILE* fp;
    size_t write_count;

    if ((filename == NULL) || (data == NULL) || (size == 0U)) {
        (void)fprintf(stderr, "Error: Invalid parameters for WriteImage\n");
        return -1;
    }

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        (void)fprintf(stderr, "Error: Failed to create output file: %s\n", filename);
        return -1;
    }

    write_count = fwrite(data, 1U, size, fp);
    if (write_count != size) {
        (void)fprintf(stderr, "Error: Failed to write complete data (wrote %zu of %zu bytes)\n",
                      write_count, size);
        (void)fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        (void)fprintf(stderr, "Error: Failed to close output file: %s\n", filename);
        return -1;
    }

    (void)printf("Wrote %zu bytes to %s\n", size, filename);
    return 0;
}
