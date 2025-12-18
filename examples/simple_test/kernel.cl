/**
 * kernel.cl - Simple OpenCL 1.2 kernel demonstrating include functionality
 */

#include "utils.h"

__kernel void simple_print(__global int* output) {
    int gid = get_global_id(0);

    printf("[kernel] Work item %d running\n", gid);

    PRINT_HELLO(gid);
    PRINT_THREAD_INFO();

    output[gid] = gid * 10;
}
