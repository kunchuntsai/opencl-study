/**
 * @file opencl_utils.h
 * @brief OpenCL utility functions for device initialization and kernel
 * execution
 *
 * Provides high-level wrappers around OpenCL API for:
 * - Platform and device initialization
 * - Kernel compilation and caching
 * - Kernel execution with timing
 * - Resource cleanup
 *
 * MISRA C 2023 Compliance:
 * - Rule 17.7: All OpenCL API return values checked
 * - Rule 21.3: Uses static buffers for kernel source
 * - Rule 22.x: Proper resource management and cleanup
 */

#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/* Include op_interface for Algorithm and OpParams */
#include "cl_extension_api.h"
#include "kernel_args.h"
#include "op_interface.h"

/**
 * @brief OpenCL environment containing all required resources
 *
 * Encapsulates all OpenCL objects needed for kernel execution:
 * platform, device, context, and command queue.
 */
typedef struct {
    cl_platform_id platform;    /**< OpenCL platform (e.g., Apple, NVIDIA) */
    cl_device_id device;        /**< OpenCL device (GPU/CPU) */
    cl_context context;         /**< OpenCL context for device */
    cl_command_queue queue;     /**< Command queue for kernel execution */
    CLExtensionContext ext_ctx; /**< Custom CL extension context */
} OpenCLEnv;

/**
 * @brief Initialize OpenCL environment
 *
 * Initializes the OpenCL platform, device, context, and command queue.
 * Automatically selects GPU if available, otherwise falls back to CPU.
 * Creates a profiling-enabled command queue for performance measurement.
 *
 * @param[out] env OpenCL environment structure to initialize
 * @return 0 on success, -1 on error
 */
int OpenclInit(OpenCLEnv* env);

/* Forward declaration for KernelConfig */
struct KernelConfig;

/**
 * @brief Build OpenCL kernel from source file with caching
 *
 * Loads kernel source, compiles it, and creates a kernel object.
 * Uses cache_manager to avoid recompilation:
 * - If cached binary exists, loads from cache
 * - Otherwise, compiles from source and saves to cache
 *
 * Headers are embedded based on #include directives in the kernel:
 * - Scans kernel for #include "xxx.h" directives
 * - Embeds matching headers from include/cl/
 * - Comments out the #include lines before compilation
 *
 * Build options are constructed as: "<user_options> -DHOST_TYPE=N"
 * where N is 0 for standard, 1 for cl_extension.
 *
 * Kernel parameters extracted from KernelConfig:
 * - kernel_file: Path to kernel source file (.cl)
 * - kernel_function: Name of kernel function in source
 * - kernel_option: User-specified build options
 * - host_type: Host type for platform selection
 *
 * @param[in] env Initialized OpenCL environment
 * @param[in] algorithm_id Algorithm identifier for cache organization
 * @param[in] kernel_cfg Kernel configuration containing file, function, options, and host type
 * @return OpenCL kernel object, or NULL on error
 */
cl_kernel OpenclBuildKernel(OpenCLEnv* env, const char* algorithm_id,
                            const struct KernelConfig* kernel_cfg);

/* OpenclSetKernelArgs is declared in kernel_args.h */

/**
 * @brief Execute OpenCL kernel with timing
 *
 * Sets kernel arguments using kernel_cfg, enqueues kernel execution, and
 * measures GPU time. All buffers and arguments are configured via .ini files.
 *
 * Uses either standard OpenCL API or custom CL extension API based on
 * the host_type configuration in the kernel config.
 *
 * Kernel execution parameters extracted from KernelConfig:
 * - global_work_size: Global work dimensions
 * - local_work_size: Local work group size
 * - work_dim: Number of work dimensions (1, 2, or 3)
 * - host_type: Host API type (standard or cl_extension)
 *
 * @param[in] env Initialized OpenCL environment
 * @param[in] kernel Compiled kernel object
 * @param[in] algo Algorithm interface (unused, kept for API compatibility)
 * @param[in] input_buf Input buffer containing image data
 * @param[out] output_buf Output buffer for processed image
 * @param[in] params Operation parameters (dimensions, algo-specific data)
 * @param[in] kernel_cfg Kernel configuration with work sizes and argument descriptors (required)
 * @param[out] gpu_time_ms Execution time in milliseconds
 * @return 0 on success, -1 on error
 */
int OpenclRunKernel(OpenCLEnv* env, cl_kernel kernel, const Algorithm* algo, cl_mem input_buf,
                    cl_mem output_buf, const OpParams* params,
                    const struct KernelConfig* kernel_cfg, double* gpu_time_ms);

/**
 * @brief Create OpenCL buffer with error checking
 *
 * Wraps clCreateBuffer with automatic error checking and logging.
 * Provides clear error messages including the buffer name for debugging.
 *
 * @param[in] context OpenCL context
 * @param[in] flags Memory flags (CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY, etc.)
 * @param[in] size Buffer size in bytes
 * @param[in] host_ptr Host pointer for initialization (can be NULL)
 * @param[in] buffer_name Descriptive name for error messages
 * @return OpenCL buffer object, or NULL on error
 */
cl_mem OpenclCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr,
                          const char* buffer_name);

/**
 * @brief Release OpenCL memory object with error checking
 *
 * Safely releases an OpenCL memory object with error checking.
 * Handles NULL pointers gracefully and logs warnings on failure.
 *
 * @param[in] mem_obj OpenCL memory object to release (can be NULL)
 * @param[in] name Descriptive name for error messages
 */
void OpenclReleaseMemObject(cl_mem mem_obj, const char* name);

/**
 * @brief Release OpenCL kernel with error checking
 *
 * Safely releases an OpenCL kernel with error checking.
 * Handles NULL pointers gracefully and logs warnings on failure.
 *
 * @param[in] kernel OpenCL kernel to release (can be NULL)
 */
void OpenclReleaseKernel(cl_kernel kernel);

/**
 * @brief Clean up OpenCL resources
 *
 * Releases all OpenCL resources in the environment structure:
 * command queue, context, device, and platform.
 *
 * @param[in,out] env OpenCL environment to clean up
 */
void OpenclCleanup(OpenCLEnv* env);
