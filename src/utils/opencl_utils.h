#ifndef OPENCL_UTILS_H
#define OPENCL_UTILS_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
} OpenCLEnv;

// Initialize OpenCL
int opencl_init(OpenCLEnv* env);

// Load and compile kernel from file
cl_kernel opencl_load_kernel(OpenCLEnv* env, const char* kernel_file,
                            const char* kernel_name);

// Execute kernel
int opencl_execute_kernel(OpenCLEnv* env, cl_kernel kernel,
                         cl_mem input_buf, cl_mem output_buf,
                         int work_dim, size_t* global_size, size_t* local_size);

// Cleanup
void opencl_cleanup(OpenCLEnv* env);

#endif // OPENCL_UTILS_H
