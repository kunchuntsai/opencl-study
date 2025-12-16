/**
 * @file platform.h
 * @brief Platform-aware function dispatch for OpenCL kernels
 *
 * Include this header in OpenCL kernels to get automatic platform-specific
 * implementations based on host_type configuration.
 *
 * The host passes -DHOST_TYPE=N during kernel compilation:
 * - HOST_TYPE=0 (PLATFORM_STANDARD): Use standard OpenCL implementations
 * - HOST_TYPE=1 (PLATFORM_CL_EXTENSION): Custom runtime provides APIs
 *
 * Usage in kernel:
 *   #include "platform.h"
 *
 *   __kernel void my_kernel(...) {
 *       // Use platform-aware functions
 *       util_mem_copy(src, dst, size);
 *   }
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/*============================================================
 * Platform Type Constants
 * Must match HostType enum in include/op_interface.h
 *============================================================*/
#define PLATFORM_STANDARD 0
#define PLATFORM_CL_EXTENSION 1

/* Default to standard if host didn't pass -DHOST_TYPE=N */
#ifndef HOST_TYPE
#define HOST_TYPE PLATFORM_STANDARD
#endif

/*============================================================
 * Platform Detection Macros
 *============================================================*/
#define IS_STANDARD_PLATFORM (HOST_TYPE == PLATFORM_STANDARD)
#define IS_CL_EXTENSION (HOST_TYPE == PLATFORM_CL_EXTENSION)

#if HOST_TYPE == PLATFORM_CL_EXTENSION
/*============================================================
 * Custom CL Extension Platform
 *
 * When using cl_extension host_type, the custom OpenCL runtime
 * provides these APIs at runtime. Nothing to define here.
 *
 * Custom platforms may provide:
 * - Hardware-accelerated DMA operations
 * - Vendor-specific intrinsics
 * - Custom memory management
 *============================================================*/

/* Placeholder for custom platform - runtime provides implementations */

#else /* PLATFORM_STANDARD */
/*============================================================
 * Standard OpenCL Platform Implementations
 *
 * These implementations use standard OpenCL APIs and work on
 * any conformant OpenCL device (NVIDIA, AMD, Intel, Apple, etc.)
 *============================================================*/

/**
 * @brief Copy memory between global buffers (work-item parallel)
 *
 * Each work-item copies one element. Launch with global_size >= count.
 *
 * @param src Source buffer
 * @param dst Destination buffer
 * @param count Number of bytes to copy
 */
inline void util_mem_copy_uchar(__global const uchar* src, __global uchar* dst, int count) {
    int gid = get_global_id(0);
    if (gid < count) {
        dst[gid] = src[gid];
    }
}

/**
 * @brief Copy memory with stride (2D copy)
 *
 * Each work-item copies one element from a 2D region.
 *
 * @param src Source buffer
 * @param dst Destination buffer
 * @param src_stride Source stride in bytes
 * @param dst_stride Destination stride in bytes
 * @param width Width of region to copy
 * @param height Height of region to copy
 */
inline void util_mem_copy_2d(__global const uchar* src, __global uchar* dst, int src_stride,
                             int dst_stride, int width, int height) {
    int gx = get_global_id(0);
    int gy = get_global_id(1);
    if (gx < width && gy < height) {
        dst[gy * dst_stride + gx] = src[gy * src_stride + gx];
    }
}

/**
 * @brief Fill buffer with constant value
 *
 * @param dst Destination buffer
 * @param value Value to fill
 * @param count Number of elements to fill
 */
inline void util_mem_set(__global uchar* dst, uchar value, int count) {
    int gid = get_global_id(0);
    if (gid < count) {
        dst[gid] = value;
    }
}

#endif /* HOST_TYPE */

#endif /* PLATFORM_H */
