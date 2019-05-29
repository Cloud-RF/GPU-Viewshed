#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>
#include <math.h>

#include "gpu.h"
#include "common.h"

const char *kernelSource =                                                                 "\n" \
"__kernel void vwshed(  __global float *heightmap,                                          \n" \
"                       __global bool *viewshed,                                            \n" \
"                       const unsigned int cols,                                            \n" \
"                       const unsigned int rows,                                            \n" \
"                       const unsigned int emitter_x,                                       \n" \
"                       const unsigned int emitter_y,                                       \n" \
"                       const unsigned int emitter_z,                                       \n" \
"                       const unsigned int radius)                                          \n" \
"{                                                                                          \n" \
"    // Get thread ID                                                                       \n" \
"    int id = get_global_id(0);                                                             \n" \
"                                                                                           \n" \
"    int x = id % cols;                                                                     \n" \
"    int y = id / cols;                                                                     \n" \
"    int cx = x;                                                                            \n" \
"    int cy = y;                                                                            \n" \
"                                                                                           \n" \
"    int dx = (x > emitter_x ? x - emitter_x : emitter_x - x), sx = x < emitter_x ? 1 : -1; \n" \
"    int dy = (y > emitter_y ? y - emitter_y : emitter_y - y), sy = y < emitter_y ? 1 : -1; \n" \
"    int dz = emitter_z + heightmap[y*cols+x];  //                                   \n" \
"    int err = (dx>dy ? dx : -dy)/2, e2;                                                    \n" \
"                                                                                           \n" \
"    bool visible = true;                                                                   \n" \
"                                                                                           \n" \
"    float distance = sqrt((float)(dx*dx + dy*dy));                                         \n" \
"    for (;;) {                                                                             \n" \
"       if(distance > radius){ visible = false; break; }                                                \n" \
"       float fraction = sqrt((float)(dx*dx + dy*dy)) / distance;                           \n" \
"       int height_offset = fraction * dz;                                         \n" \
"       if (heightmap[cy*cols+cx] > height_offset){ visible = false; }\n" \
"       if (cx == emitter_x && cy == emitter_y){ break; }                                   \n" \
"       if (sqrt((float)(cx*cx + cy*cy)) > cols){ break; }                                   \n" \
"       e2 = err;                                                                           \n" \
"       if (e2 > -dx) { err -= dy; cx += sx; }                                              \n" \
"       if (e2 <  dy) { err += dx; cy += sy; }                                              \n" \
"    }                                                                                      \n" \
"    viewshed[y*cols+x] = visible;                                                          \n" \
"    return;                                                                                \n" \
"}                                                                                          \n" \
                                                                                           "\n" ;
vs_viewshed_t gpu_calculate_viewshed(vs_heightmap_t heightmap, uint32_t emitter_x, uint32_t emitter_y, uint32_t emitter_z, uint32_t radius){
    vs_viewshed_t viewshed = viewshed_from_heightmap(heightmap);

    // Device buffers
    cl_mem d_heightmap;
    cl_mem d_viewshed;

    cl_platform_id cpPlatform;        // OpenCL platform
    cl_device_id device_id;           // device ID
    cl_context context;               // context
    cl_command_queue queue;           // command queue
    cl_program program;               // program
    cl_kernel kernel;                 // kernel

    size_t globalSize, localSize;
    cl_int err;

    // Number of work items in each local work group
    localSize = 1000;

    // Number of total work items - localSize must be devisor
    globalSize = ceil(heightmap.cols*heightmap.rows/(float)localSize)*localSize;

    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Binding to platform failed: %d\n", err);
        return viewshed;
    }

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Fetching device IDs failed: %d\n", err);
        return viewshed;
    }

    // Create a context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Creating context failed: %d\n", err);
        return viewshed;
    }

    // Create a command queue
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Creating command queue failed: %d\n", err);
        return viewshed;
    }

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) & kernelSource, NULL, &err);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Reading source for kernel failed: %d\n", err);
        return viewshed;
    }

    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Building kernel failed: %d\n", err);

        size_t len = 0;
        cl_int ret = CL_SUCCESS;
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        char *buffer = calloc(len, sizeof(char));
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
        fprintf(stderr, "Build log:\n\n%s\n", buffer);

        return viewshed;
    }

    // Create the compute kernel in the program we wish to run
    kernel = clCreateKernel(program, "vwshed", &err);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Creating kernel failed: %d\n", err);
        return viewshed;
    }

    // Create the input and output arrays in device memory for our calculation
    int errcode_ret = 0;
    d_heightmap = clCreateBuffer(context, CL_MEM_READ_ONLY, heightmap.rows*heightmap.cols*sizeof(float), NULL, &errcode_ret);
    if( errcode_ret != CL_SUCCESS ){
        fprintf(stderr, "Creating heightmap data buffer failed: %d\n", errcode_ret);
        return viewshed;
    }
    d_viewshed  = clCreateBuffer(context, CL_MEM_WRITE_ONLY, heightmap.rows*heightmap.cols*sizeof(bool), NULL, &errcode_ret);
    if( errcode_ret != CL_SUCCESS ){
        fprintf(stderr, "Creating viewshed data buffer failed: %d\n", errcode_ret);
        return viewshed;
    }

    fprintf(stderr, "x: %d y: %d z: %d Terrain at source: %f\n", emitter_x, emitter_y, emitter_z, heightmap.heightmap[emitter_y*heightmap.cols+emitter_x]);
    // Write our data set into the input array in device memory
    err = clEnqueueWriteBuffer(queue, d_heightmap, CL_TRUE, 0, viewshed.cols*viewshed.rows*sizeof(*heightmap.heightmap), heightmap.heightmap, 0, NULL, NULL);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Enqueing data buffer writes failed: %d\n", err);
        return viewshed;
    }

    // Set the kernel arguments
    if( (err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&d_heightmap)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 0): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&d_viewshed)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 1): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 2, sizeof(uint32_t), (void *)&heightmap.cols)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 2): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 3, sizeof(uint32_t), (void *)&heightmap.rows)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 3): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 4, sizeof(uint32_t), (void *)&emitter_x)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 4): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 5, sizeof(uint32_t), (void *)&emitter_y)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 5): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 6, sizeof(uint32_t), (void *)&emitter_z)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 6): %d\n", err);
        return viewshed;
    }else if( (err = clSetKernelArg(kernel, 7, sizeof(uint32_t), (void *)&radius)) != CL_SUCCESS ){
        fprintf(stderr, "Binding to platform failed (arg 7): %d\n", err);
        return viewshed;
    }

    // Execute the kernel over the entire range of the data set
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
    if (err != CL_SUCCESS){
        fprintf(stderr, "Executing kernel failed: %d\n", err);
        return viewshed;
    }

    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);

    // Read the results from the device
    err = clEnqueueReadBuffer(queue, d_viewshed, CL_TRUE, 0, viewshed.cols*viewshed.rows*sizeof(bool), viewshed.viewshed, 0, NULL, NULL );
    if (err != CL_SUCCESS){
        fprintf(stderr, "Reading back viewshed failed: %d\n", err);
        return viewshed;
    }

    // Release OpenCL resources
    clReleaseMemObject(d_viewshed);
    clReleaseMemObject(d_heightmap);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return viewshed;
}

void gpu_curve_map(vs_heightmap_t map){
    return;
}
