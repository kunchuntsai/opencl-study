/**
 * utils.h - Kernel utility functions for OpenCL 1.2
 *
 * This header can be included by OpenCL kernels to provide common utility functions.
 * Uses OpenCL 1.2 printf extension for output.
 */

#ifndef UTILS_H
#define UTILS_H

/**
 * Print a hello message from the kernel.
 * Uses OpenCL 1.2 printf extension.
 *
 * @param work_item_id The global work item ID to include in the message
 */
inline void print_hello(int work_item_id) {
    printf("[utils.h] Hello from work item %d!\n", work_item_id);
}

/**
 * Print a custom message with a value.
 *
 * @param msg A message string (limited to what OpenCL printf supports)
 * @param value An integer value to print
 */
inline void print_value(const char* msg, int value) {
    printf("[utils.h] %s: %d\n", msg, value);
}

/**
 * Simple utility to get thread info and print it.
 */
inline void print_thread_info(void) {
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int group_id = get_group_id(0);
    printf("[utils.h] Thread info - global_id: %d, local_id: %d, group_id: %d\n",
           gid, lid, group_id);
}

#endif /* UTILS_H */
