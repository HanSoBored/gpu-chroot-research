/*
 * Corrected OpenCL test with standard constants - Stage 2
 * Uses direct dlopen/dlsym to load OpenCL functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

// OpenCL type definitions
typedef unsigned int cl_uint;
typedef int cl_int;
typedef void* cl_platform_id;
typedef void* cl_device_id;
typedef void* cl_context;
typedef void* cl_command_queue;
typedef void* cl_program;
typedef void* cl_kernel;
typedef void* cl_mem;
typedef size_t cl_ulong;
typedef size_t size_t_cl;

// OpenCL error codes
#define CL_SUCCESS 0
#define CL_DEVICE_TYPE_GPU (1 << 1)
#define CL_DEVICE_TYPE_ALL 0xFFFFFFFF

// OpenCL info constants
#define CL_PLATFORM_NAME 0x0902
#define CL_DEVICE_NAME 0x102B
#define CL_DEVICE_VENDOR 0x102C
#define CL_DEVICE_TYPE 0x1000

// CORRECTED Memory flags (Stage 2 fix)
#define CL_MEM_READ_WRITE (1 << 0)
#define CL_MEM_WRITE_ONLY (1 << 1)
#define CL_MEM_READ_ONLY (1 << 2)
#define CL_MEM_USE_HOST_PTR (1 << 3)
#define CL_MEM_ALLOC_HOST_PTR (1 << 4)
#define CL_MEM_COPY_HOST_PTR (1 << 5)

// OpenCL function pointer types
typedef cl_int (*clGetPlatformIDs_t)(cl_uint, cl_platform_id*, cl_uint*);
typedef cl_int (*clGetPlatformInfo_t)(cl_platform_id, cl_uint, size_t_cl, void*, size_t_cl*);
typedef cl_int (*clGetDeviceIDs_t)(cl_platform_id, cl_uint, cl_uint, cl_device_id*, cl_uint*);
typedef cl_int (*clGetDeviceInfo_t)(cl_device_id, cl_uint, size_t_cl, void*, size_t_cl*);
typedef cl_context (*clCreateContext_t)(const void*, cl_uint, const cl_device_id*, void*, void*, cl_int*);
typedef cl_command_queue (*clCreateCommandQueue_t)(cl_context, cl_device_id, cl_ulong, cl_int*);
typedef cl_mem (*clCreateBuffer_t)(cl_context, cl_ulong, size_t_cl, void*, cl_int*);
typedef cl_program (*clCreateProgramWithSource_t)(cl_context, cl_uint, const char**, const size_t_cl*, cl_int*);
typedef cl_int (*clBuildProgram_t)(cl_program, cl_uint, const cl_device_id*, const char*, void*, void*);
typedef cl_kernel (*clCreateKernel_t)(cl_program, const char*, cl_int*);
typedef cl_int (*clSetKernelArg_t)(cl_kernel, cl_uint, size_t_cl, const void*);
typedef cl_int (*clEnqueueNDRangeKernel_t)(cl_command_queue, cl_kernel, cl_uint, const size_t_cl*, const size_t_cl*, const size_t_cl*, cl_uint, const void*, void*);
typedef cl_int (*clEnqueueReadBuffer_t)(cl_command_queue, cl_mem, cl_uint, size_t_cl, size_t_cl, void*, cl_uint, const void*, void*);
typedef cl_int (*clFinish_t)(cl_command_queue);
typedef cl_int (*clReleaseKernel_t)(cl_kernel);
typedef cl_int (*clReleaseProgram_t)(cl_program);
typedef cl_int (*clReleaseCommandQueue_t)(cl_command_queue);
typedef cl_int (*clReleaseContext_t)(cl_context);
typedef cl_int (*clReleaseMemObject_t)(cl_mem);

// Function pointers
static void* cl_lib = NULL;
static clGetPlatformIDs_t fn_clGetPlatformIDs = NULL;
static clGetPlatformInfo_t fn_clGetPlatformInfo = NULL;
static clGetDeviceIDs_t fn_clGetDeviceIDs = NULL;
static clGetDeviceInfo_t fn_clGetDeviceInfo = NULL;
static clCreateContext_t fn_clCreateContext = NULL;
static clCreateCommandQueue_t fn_clCreateCommandQueue = NULL;
static clCreateBuffer_t fn_clCreateBuffer = NULL;
static clCreateProgramWithSource_t fn_clCreateProgramWithSource = NULL;
static clBuildProgram_t fn_clBuildProgram = NULL;
static clCreateKernel_t fn_clCreateKernel = NULL;
static clSetKernelArg_t fn_clSetKernelArg = NULL;
static clEnqueueNDRangeKernel_t fn_clEnqueueNDRangeKernel = NULL;
static clEnqueueReadBuffer_t fn_clEnqueueReadBuffer = NULL;
static clFinish_t fn_clFinish = NULL;
static clReleaseKernel_t fn_clReleaseKernel = NULL;
static clReleaseProgram_t fn_clReleaseProgram = NULL;
static clReleaseCommandQueue_t fn_clReleaseCommandQueue = NULL;
static clReleaseContext_t fn_clReleaseContext = NULL;
static clReleaseMemObject_t fn_clReleaseMemObject = NULL;

static int load_opencl_functions() {
    cl_lib = dlopen("libOpenCL.so", RTLD_LAZY);
    if (!cl_lib) {
        printf("ERROR: Could not load libOpenCL.so: %s\n", dlerror());
        return 0;
    }

    #define LOAD_FUNC(name) \
        fn_##name = (name##_t)dlsym(cl_lib, #name); \
        if (!fn_##name) { \
            printf("ERROR: Could not load %s: %s\n", #name, dlerror()); \
            return 0; \
        }

    LOAD_FUNC(clGetPlatformIDs);
    LOAD_FUNC(clGetPlatformInfo);
    LOAD_FUNC(clGetDeviceIDs);
    LOAD_FUNC(clGetDeviceInfo);
    LOAD_FUNC(clCreateContext);
    LOAD_FUNC(clCreateCommandQueue);
    LOAD_FUNC(clCreateBuffer);
    LOAD_FUNC(clCreateProgramWithSource);
    LOAD_FUNC(clBuildProgram);
    LOAD_FUNC(clCreateKernel);
    LOAD_FUNC(clSetKernelArg);
    LOAD_FUNC(clEnqueueNDRangeKernel);
    LOAD_FUNC(clEnqueueReadBuffer);
    LOAD_FUNC(clFinish);
    LOAD_FUNC(clReleaseKernel);
    LOAD_FUNC(clReleaseProgram);
    LOAD_FUNC(clReleaseCommandQueue);
    LOAD_FUNC(clReleaseContext);
    LOAD_FUNC(clReleaseMemObject);

    #undef LOAD_FUNC

    printf("All OpenCL functions loaded successfully\n");
    return 1;
}

// Simple OpenCL kernel: vector addition
const char* kernel_source =
"__kernel void vec_add(__global const float* a, "
"                      __global const float* b, "
"                      __global float* c) {"
"    int id = get_global_id(0);"
"    c[id] = a[id] + b[id];"
"}";

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buf_a, buf_b, buf_c;

    const int SIZE = 1024;
    float *a, *b, *c;

    printf("=== OpenCL GPU Compute Test (Stage 2 - Fixed) ===\n\n");

    // Load OpenCL functions
    printf("Step 0: Loading OpenCL functions...\n");
    if (!load_opencl_functions()) {
        printf("FAILED to load OpenCL!\n");
        printf("This means libOpenCL.so is not accessible or incompatible.\n");
        return 1;
    }

    // Allocate host memory
    a = (float*)malloc(SIZE * sizeof(float));
    b = (float*)malloc(SIZE * sizeof(float));
    c = (float*)malloc(SIZE * sizeof(float));

    if (!a || !b || !c) {
        printf("ERROR: Failed to allocate host memory\n");
        return 1;
    }

    // Initialize data
    for (int i = 0; i < SIZE; i++) {
        a[i] = i * 1.5f;
        b[i] = i * 2.5f;
    }

    printf("\nStep 1: Get OpenCL platforms...\n");
    cl_uint num_platforms;
    err = fn_clGetPlatformIDs(1, &platform, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        printf("ERROR: No OpenCL platforms found (err=%d)\n", err);
        printf("This means GPU driver is not accessible!\n");
        return 1;
    }
    printf("  Found %d platform(s)\n", num_platforms);

    // Get platform info
    char platform_name[256];
    fn_clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
    printf("  Platform: %s\n", platform_name);

    printf("\nStep 2: Get GPU device...\n");
    err = fn_clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: No GPU device found (err=%d)\n", err);
        printf("Trying ALL devices...\n");
        err = fn_clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
        if (err != CL_SUCCESS) {
            printf("ERROR: No devices found at all!\n");
            return 1;
        }
    }

    // Get device info
    char device_name[256];
    fn_clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("  Device: %s\n", device_name);

    char device_vendor[256];
    fn_clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
    printf("  Vendor: %s\n", device_vendor);

    printf("\nStep 3: Create OpenCL context...\n");
    context = fn_clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create context (err=%d)\n", err);
        return 1;
    }
    printf("  Context created successfully\n");

    printf("\nStep 4: Create command queue...\n");
    queue = fn_clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create command queue (err=%d)\n", err);
        return 1;
    }
    printf("  Command queue created\n");

    printf("\nStep 5: Create buffers...\n");
    buf_a = fn_clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           SIZE * sizeof(float), a, &err);
    buf_b = fn_clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           SIZE * sizeof(float), b, &err);
    buf_c = fn_clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                           SIZE * sizeof(float), NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create buffers (err=%d)\n", err);
        return 1;
    }
    printf("  Buffers created (%d floats each)\n", SIZE);

    printf("\nStep 6: Build kernel program...\n");
    program = fn_clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create program (err=%d)\n", err);
        return 1;
    }

    err = fn_clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to build program (err=%d)\n", err);
        return 1;
    }
    printf("  Program built successfully\n");

    printf("\nStep 7: Create kernel...\n");
    kernel = fn_clCreateKernel(program, "vec_add", &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create kernel (err=%d)\n", err);
        return 1;
    }
    printf("  Kernel created\n");

    printf("\nStep 8: Set kernel arguments...\n");
    fn_clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a);
    fn_clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b);
    fn_clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_c);
    printf("  Arguments set\n");

    printf("\nStep 9: Execute kernel on GPU...\n");
    size_t_cl global_size = SIZE;
    err = fn_clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to execute kernel (err=%d)\n", err);
        return 1;
    }
    printf("  Kernel execution queued\n");

    printf("\nStep 9b: Waiting for kernel to finish...\n");
    err = fn_clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("ERROR: clFinish failed (err=%d)\n", err);
    } else {
        printf("  Kernel execution completed\n");
    }

    printf("\nStep 10: Read results...\n");
    err = fn_clEnqueueReadBuffer(queue, buf_c, 1, 0, SIZE * sizeof(float), c, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to read results (err=%d)\n", err);
        return 1;
    }
    printf("  Results read back\n");

    printf("\nStep 11: Verify results...\n");
    int errors = 0;
    for (int i = 0; i < SIZE && errors < 5; i++) {
        float expected = a[i] + b[i];
        if (c[i] != expected) {
            printf("  ERROR at [%d]: expected %f, got %f\n", i, expected, c[i]);
            errors++;
        }
    }

    printf("\n=== RESULTS ===\n");
    if (errors == 0) {
        printf("SUCCESS! All %d computations correct!\n", SIZE);
        printf("\nGPU Compute is WORKING!\n");
        printf("Sample results (first 10):\n");
        for (int i = 0; i < 10; i++) {
            printf("  a[%d]=%f + b[%d]=%f = c[%d]=%f\n",
                   i, a[i], i, b[i], i, c[i]);
        }
    } else {
        printf("FAILED! %d errors found\n", errors);
        printf("\nNote: This is expected behavior in chroot.\n");
        printf("GPU commands are queued but not executed due to\n");
        printf("missing Android framework services (Binder, HAL, etc).\n");
    }

    // Cleanup
    fn_clReleaseKernel(kernel);
    fn_clReleaseProgram(program);
    fn_clReleaseCommandQueue(queue);
    fn_clReleaseContext(context);
    fn_clReleaseMemObject(buf_a);
    fn_clReleaseMemObject(buf_b);
    fn_clReleaseMemObject(buf_c);
    free(a);
    free(b);
    free(c);

    if (cl_lib) dlclose(cl_lib);

    return errors > 0 ? 1 : 0;
}
