#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "opencl_manager.hpp"

// helpers
void clcheck(cl_int status, std::string str) {
    if (status != CL_SUCCESS) {
        switch (status) {
            case CL_INVALID_CONTEXT: {
                std::cout << "Invalid Context ";
                break; }
            case CL_INVALID_VALUE: {
                std::cout << "Invalid Value ";
                break; }
            case CL_INVALID_EVENT_WAIT_LIST: {
                std::cout << "Invalid Event Wait List ";
                break; }
            case CL_MISALIGNED_SUB_BUFFER_OFFSET: {
                std::cout << "Misaligned Sub Buffer Offset ";
                break; }
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: {
                std::cout << "Memory Object Allocation Failure ";
                break; }
            case CL_OUT_OF_RESOURCES: {
                std::cout << "Out of Resources ";
                break; }
            case CL_OUT_OF_HOST_MEMORY: {
                std::cout << "Out of Host Memory ";
                break; }
            case CL_INVALID_MEM_OBJECT: {
                std::cout << "Invalid Mem Object ";
                break; }
        }
        std::cout << "Error: ";
        std::cout << str << std::endl;
    }
}

// constructor
OpenCL_Manager::OpenCL_Manager() {
}

// initialize opencl
int OpenCL_Manager::initialize() {
    
    // setup platform
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    
    if(all_platforms.size()==0){
        std::cout<<" No platforms found."<<std::endl;
        exit(1);
    }
    
    m_platform=all_platforms[0];
    
    // setup device of type GPU
    std::vector<cl::Device> all_devices;
    m_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size()==0) {
        std::cout << "No GPU Devices found." << std::endl;
        exit(1);
    }
    
    m_device=all_devices[0];
    
    // setup context
    m_context = cl::Context({m_device});
    
    return 0;
}

int OpenCL_Manager::kernel_load(std::string file_name,
    std::string kernel_name) {
    
    // setup program
    std::ifstream stream(file_name);
    
    if (!stream.is_open()) {
        std::cout << "Cannot open kernel." << std::endl;
        exit(1);
    }
    
    std::string source = std::string(std::istreambuf_iterator<char>(stream),
                                     std::istreambuf_iterator<char>());
    
    m_program = cl::Program(m_context,source,true);
    
    cl_int err = m_program.build({m_device});

    if(err != CL_SUCCESS) {
//        if(err == CL_INVALID_BINARY ) {
//            std::cout << "a" << std::endl;
//        }
//        else if (err == CL_INVALID_OPERATION ) {
//            std::cout << "b" << std::endl;
//        }
//        else if (err == CL_BUILD_PROGRAM_FAILURE ) {
//            std::cout << "c" << std::endl;
//        }
        std::cout << "Build Status: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>({m_device}) << std::endl;
        std::cout << "Build Options:\t" << m_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>({m_device}) << std::endl;
        std::cout << "Build Log:\t " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>({m_device}) << std::endl;
        exit(1);
    }

    // setup kernel
    m_kernel = cl::Kernel(m_program,kernel_name.c_str());
    
    return 0;
}



int OpenCL_Manager::kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                                   Pixel *image, int width, int height, int* seed_memory,
                                   int iterations, Triangle *triangles, int n_triangles,
                                   Material *materials, int n_materials) {
    
    
    // [ Limits ]
    size_t maxWorkGrpSize = 5;
    std::vector<size_t> workItemSizes;
    
    clcheck(m_device.getInfo<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGrpSize),
         "Max Work Group Size Query");
    m_device.getInfo<std::vector<size_t> >(CL_DEVICE_MAX_WORK_ITEM_SIZES, &workItemSizes);

    
    // setup data & buffers & local args
    
    // [ Global ]
    cl::Buffer prim_buff(m_context,CL_MEM_READ_WRITE,sizeof(Primitive)*n_primitives);
    cl::Buffer camera_buff(m_context,CL_MEM_READ_ONLY,sizeof(Camera));
    cl::Buffer image_buff(m_context,CL_MEM_WRITE_ONLY,sizeof(Pixel)*width*height);
    cl::Buffer seed_memory_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);
    cl::Buffer tri_buff(m_context,CL_MEM_READ_ONLY,sizeof(Triangle)*n_triangles);
    cl::Buffer mat_buff(m_context,CL_MEM_READ_ONLY,sizeof(Material)*n_materials);
    
    // [ Local ]
    cl::LocalSpaceArg local_prim_buff = cl::Local(sizeof(Primitive)*n_primitives);
    cl::LocalSpaceArg local_mat_buff = cl::Local(sizeof(Material)*n_materials);
    cl::LocalSpaceArg local_tri_buff = cl::Local(sizeof(Triangle)*n_triangles);
    
    // [ Command Queue ]
    m_queue = cl::CommandQueue(m_context,m_device);
    
    // [ Set Kernel Arguments ]
    clcheck(m_queue.enqueueWriteBuffer(prim_buff,CL_TRUE,0,sizeof(Primitive)*n_primitives, primitives),
          "prim_buff write");
    clcheck(m_queue.enqueueWriteBuffer(camera_buff,CL_TRUE,0,sizeof(Camera),camera),
          "camera_buff write");
    clcheck(m_queue.enqueueWriteBuffer(image_buff, CL_TRUE, 0, sizeof(Pixel)*width*height, image),
          "image_buff write");
    clcheck(m_queue.enqueueWriteBuffer(seed_memory_buff, CL_TRUE, 0, sizeof(int)*width*height, seed_memory),
            "seed_memory_buff write");
    clcheck(m_queue.enqueueWriteBuffer(tri_buff, CL_TRUE, 0, sizeof(Triangle)*n_triangles, triangles),
            "tri_buff write");
    clcheck(m_queue.enqueueWriteBuffer(mat_buff, CL_TRUE, 0, sizeof(Material)*n_materials, materials),
            "mat_memory_buff write");
    
    //const __global float4* global_primitives,
    //const __global Triangle* triangles,
    //const __global float4* global_materials,
    //__global Pixel* image,
    //__global int* seed_memory,
    //__global Camera* camera,
    //__local float4* local_primitives,
    //__local float4* local_materials,
    //int width,
    //int height,
    //int iteration,
    //int n_triangles,
    //int n_materials,
    //int n_primitives
    
    // Global
    clcheck(m_kernel.setArg(0,prim_buff),"Global Primitives");
    clcheck(m_kernel.setArg(1,tri_buff),"Global Triangles");
    clcheck(m_kernel.setArg(2,mat_buff),"Global Materials");
    clcheck(m_kernel.setArg(3,image_buff),"Global Image");
    clcheck(m_kernel.setArg(4,seed_memory_buff),"Global Seed Memory");
    clcheck(m_kernel.setArg(5,camera_buff),"Global Camera");
    // Locals
    clcheck(m_kernel.setArg(6,local_prim_buff),"Local Primitives");
    clcheck(m_kernel.setArg(7,local_mat_buff),"Local Materials");
    clcheck(m_kernel.setArg(8,local_tri_buff),"Local Triangles");
    // Scalars
    clcheck(m_kernel.setArg(9,width),"set width");
    clcheck(m_kernel.setArg(10,height),"set height");
    clcheck(m_kernel.setArg(11,iterations),"set iterations");
    clcheck(m_kernel.setArg(12,n_triangles),"set n_triangles");
    clcheck(m_kernel.setArg(13,n_materials),"set n_materials");
    clcheck(m_kernel.setArg(14,n_primitives),"set n_primitives");
    
    int global_size, size = width*height;
    
    // global work size must be evenly divisible by work item size
    if (size % workItemSizes[0] != 0) {
        global_size = (size/workItemSizes[0]) * workItemSizes[0]
        + workItemSizes[0];
    }
    else {
        global_size = size;
    }
    
    
    // offset, global work size, local work size
    clcheck(m_queue.enqueueNDRangeKernel(m_kernel,
                                      cl::NullRange,
                                      cl::NDRange(global_size),
                                      cl::NDRange(workItemSizes[0])),
            "enqueueNDRangeKernel");
    
    // blocks program until done
    m_queue.finish();
    m_queue.enqueueReadBuffer(image_buff,CL_TRUE,0,sizeof(Pixel)*width*height,image);
    m_queue.enqueueReadBuffer(seed_memory_buff,CL_TRUE,0,sizeof(int)*width*height,seed_memory);
    
    return 0;
}