/**
 * kernel.cl - Simple OpenCL 1.2 kernel demonstrating include functionality
 *
 * This kernel includes utils.h to use shared utility functions.
 */

/* Enable OpenCL 1.2 printf extension */
#pragma OPENCL EXTENSION cl_khr_fp64 : disable

/* Include our utility header */
#include "utils.h"

/**
 * Simple kernel that demonstrates:
 * 1. Including a header file (utils.h)
 * 2. Using functions from the included header
 * 3. OpenCL 1.2 printf for output
 */
__kernel void simple_print(__global int* output) {
    int gid = get_global_id(0);

    /* Print directly from kernel */
    printf("[kernel.cl] Kernel running on work item %d\n", gid);

    /* Use utility function from utils.h */
    print_hello(gid);

    /* Use another utility function */
    print_value("Work item ID", gid);

    /* Print thread info using utility */
    print_thread_info();

    /* Write to output buffer to prove kernel executed */
    output[gid] = gid * 10;

    printf("[kernel.cl] Work item %d wrote value %d to output\n", gid, gid * 10);
}
