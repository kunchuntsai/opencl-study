/**
 * Minimal OpenCL 1.2 header stub for compilation testing
 *
 * NOTE: This is a minimal stub for testing compilation.
 * For actual OpenCL runtime, install opencl-headers and ocl-icd-opencl-dev packages.
 *
 * On Ubuntu/Debian:
 *   sudo apt-get install opencl-headers ocl-icd-opencl-dev
 *
 * On macOS:
 *   OpenCL is included in the system frameworks
 */

#ifndef __OPENCL_CL_H
#define __OPENCL_CL_H

#include <stddef.h>
#include <stdint.h>

/* OpenCL scalar types */
typedef int8_t   cl_char;
typedef uint8_t  cl_uchar;
typedef int16_t  cl_short;
typedef uint16_t cl_ushort;
typedef int32_t  cl_int;
typedef uint32_t cl_uint;
typedef int64_t  cl_long;
typedef uint64_t cl_ulong;
typedef uint16_t cl_half;
typedef float    cl_float;
typedef double   cl_double;

/* OpenCL handle types */
typedef struct _cl_platform_id*    cl_platform_id;
typedef struct _cl_device_id*      cl_device_id;
typedef struct _cl_context*        cl_context;
typedef struct _cl_command_queue*  cl_command_queue;
typedef struct _cl_mem*            cl_mem;
typedef struct _cl_program*        cl_program;
typedef struct _cl_kernel*         cl_kernel;
typedef struct _cl_event*          cl_event;
typedef struct _cl_sampler*        cl_sampler;

/* OpenCL bitfield types */
typedef cl_ulong cl_bitfield;
typedef cl_bitfield cl_mem_flags;
typedef cl_bitfield cl_device_type;
typedef cl_bitfield cl_command_queue_properties;

/* Error codes */
#define CL_SUCCESS                          0
#define CL_BUILD_PROGRAM_FAILURE           -11
#define CL_COMPILE_PROGRAM_FAILURE         -15
#define CL_INVALID_PROGRAM                 -44
#define CL_INVALID_KERNEL_NAME             -46
#define CL_INVALID_VALUE                   -30

/* Device types */
#define CL_DEVICE_TYPE_CPU                  (1 << 1)
#define CL_DEVICE_TYPE_GPU                  (1 << 2)

/* Device info */
#define CL_DEVICE_NAME                      0x102B

/* Memory flags */
#define CL_MEM_READ_WRITE                   (1 << 0)
#define CL_MEM_WRITE_ONLY                   (1 << 1)
#define CL_MEM_READ_ONLY                    (1 << 2)
#define CL_MEM_COPY_HOST_PTR                (1 << 5)

/* Program build info */
#define CL_PROGRAM_BUILD_LOG                0x1183

/* Boolean */
#define CL_TRUE                             1
#define CL_FALSE                            0

/* OpenCL 1.2 API declarations */
extern cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);
extern cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);
extern cl_int clGetDeviceInfo(cl_device_id device, cl_uint param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
extern cl_context clCreateContext(const void* properties, cl_uint num_devices, const cl_device_id* devices, void (*pfn_notify)(const char*, const void*, size_t, void*), void* user_data, cl_int* errcode_ret);
extern cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret);
extern cl_program clCreateProgramWithSource(cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret);
extern cl_int clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (*pfn_notify)(cl_program, void*), void* user_data);
extern cl_int clGetProgramBuildInfo(cl_program program, cl_device_id device, cl_uint param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
extern cl_kernel clCreateKernel(cl_program program, const char* kernel_name, cl_int* errcode_ret);
extern cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret);
extern cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value);
extern cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
extern cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_uint blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
extern cl_int clFinish(cl_command_queue command_queue);
extern cl_int clReleaseMemObject(cl_mem memobj);
extern cl_int clReleaseKernel(cl_kernel kernel);
extern cl_int clReleaseProgram(cl_program program);
extern cl_int clReleaseCommandQueue(cl_command_queue command_queue);
extern cl_int clReleaseContext(cl_context context);

#endif /* __OPENCL_CL_H */
