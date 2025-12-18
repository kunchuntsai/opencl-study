/**
 * cl/utils.h - Simple utility macros for OpenCL kernels
 */

#ifndef CL_UTILS_H
#define CL_UTILS_H

#define PRINT_HELLO(gid) printf("[utils] Hello from work item %d\n", gid)

#define PRINT_THREAD_INFO() \
    printf("[utils] gid=%d lid=%d group=%d\n", \
           (int)get_global_id(0), (int)get_local_id(0), (int)get_group_id(0))

#endif
