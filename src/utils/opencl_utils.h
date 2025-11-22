#pragma once

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

/* Initialize OpenCL environment (platform, device, context, queue) */
int opencl_init(OpenCLEnv* env);

/* Build kernel from source file (load and compile) */
cl_kernel opencl_build_kernel(OpenCLEnv* env, const char* kernel_file,
                               const char* kernel_name);

/* Run kernel with provided buffers and configuration */
int opencl_run_kernel(OpenCLEnv* env, cl_kernel kernel,
                      cl_mem input_buf, cl_mem output_buf,
                      int width, int height,
                      const size_t* global_work_size,
                      const size_t* local_work_size,
                      int work_dim,
                      double* gpu_time_ms);

/* Cleanup OpenCL resources */
void opencl_cleanup(OpenCLEnv* env);
