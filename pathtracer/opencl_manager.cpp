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
int OpenCL_Manager::initialize(int width, int height) {
    
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
    
    // setup limits
    m_global_size = width*height;
    
    size_t maxWorkGrpSize = 5;
    std::vector<size_t> workItemSizes;
    
    clcheck(m_device.getInfo<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGrpSize),
            "Max Work Group Size Query");
    m_device.getInfo<std::vector<size_t> >(CL_DEVICE_MAX_WORK_ITEM_SIZES, &workItemSizes);
    
    m_work_item_size = workItemSizes[0];

    int size = m_global_size;
    
    // global work size must be evenly divisible by work item size
    if (size % workItemSizes[0] != 0) {
        m_global_size = (size/workItemSizes[0]) * workItemSizes[0]
        + workItemSizes[0];
    }
    else {
        m_global_size = size;
    }
    
    size_t max_width;
    
    // N E W    C O D E
    // load all the fancy new kernels
    new_kernel_load(1, "initial_ray_kernel.cl", "initial_ray_kernel");
    new_kernel_load(2, "ray_kernel.cl", "ray_kernel");
    new_kernel_load(3, "reconstruct_kernel.cl", "reconstruct_kernel");
    std::cout << "All Kernels Loaded\n";
    
    return 1;
}

int OpenCL_Manager::new_kernel_load(int kid,
                    std::string file_name,
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
        std::cout << "Build Status: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>({m_device}) << std::endl;
        std::cout << "Build Options:\t" << m_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>({m_device}) << std::endl;
        std::cout << "Build Log:\t " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>({m_device}) << std::endl;
        exit(1);
    }
    
    // setup kernel
    switch (kid) {
        case 1:
            initial_ray_kernel = cl::Kernel(m_program,kernel_name.c_str());
            break;
        case 2:
            ray_kernel = cl::Kernel(m_program,kernel_name.c_str());
            break;
        case 3:
            reconstruct_kernel = cl::Kernel(m_program,kernel_name.c_str());
            break;
        default:
            break;
    }
    
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

void print_f4(cl_float4 f4){
    std::cout << f4.x << " ";
    std::cout << f4.y << " ";
    std::cout << f4.z << " ";
}

void print_tri(Triangle tri) {
    print_f4(tri.v1);
    std::cout << " | ";
    print_f4(tri.e1);
    std::cout << " | ";
    print_f4(tri.e2);
    std::cout << "\n";
}


// load triangles to host memory
int OpenCL_Manager::triangles_to_texture(Triangle *triangles,int n_triangles) {

    size_t width = n_triangles;
    std::cout << "Triangles to Texture "<< n_triangles <<"\n";
    
    cl::ImageFormat format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_FLOAT;
    
    // create 1D texture
    m_tri_texture = cl::Image1D(m_context,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            format,width*3,triangles);
    
    return 0;
}

int OpenCL_Manager::bvh_to_texture(BVH_Node_BBox *bbox,
                                   BVH_Node_Info *info,
                                   int n_nodes) {
    
    size_t width = n_nodes;

    // [ Bbox ]
    cl::ImageFormat bboxformat;
    
    bboxformat.image_channel_order = CL_RGB;
    bboxformat.image_channel_data_type = CL_FLOAT;
    
    // create 1D texture
    cl_int error;
    m_bvh_bbox = cl::Image1D(m_context,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             bboxformat,width*2,bbox,&error);
    
    // [ Info ]
    cl::ImageFormat infoformat;
    infoformat.image_channel_order = CL_RGBA;
    infoformat.image_channel_data_type =  CL_SIGNED_INT32;
    
    m_bvh_info = cl::Image1D(m_context,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             infoformat,width,info,&error);
    
    if (error != NULL) {
        std::cout << "Failed to create BVH Images\n";
    }

    return 0;
}


//first raycast
int OpenCL_Manager::initial_ray_kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                                              int width, int height, int* seed_memory,
                                              int iterations, Triangle *triangles, int n_triangles,
                                              Material *materials, int n_materials,
                                              BVH_Node *nodes, int n_nodes, cl_float4 *color,
                                              cl_float4 *origin, cl_float4 *direction, cl_float4 *weight) {
    
    // [ Global ]
    cl::Buffer prim_buff(m_context,CL_MEM_READ_WRITE,sizeof(Primitive)*n_primitives);
    cl::Buffer camera_buff(m_context,CL_MEM_READ_ONLY,sizeof(Camera));
    
    cl::Buffer seed_memory_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);
    
    // information
    cl::Buffer origin_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer direction_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer weight_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer color_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    
    cl::Buffer mat_buff(m_context,CL_MEM_READ_ONLY,sizeof(Material)*n_materials);
    cl::Buffer bvh_buff(m_context,CL_MEM_READ_ONLY,sizeof(BVH_Node)*n_nodes);
    
    // [ Local ]
    cl::LocalSpaceArg local_prim_buff = cl::Local(sizeof(Primitive)*n_primitives);
    cl::LocalSpaceArg local_mat_buff = cl::Local(sizeof(Material)*n_materials);
    
    // [ Command Queue ]
    m_queue = cl::CommandQueue(m_context,m_device);
    
    // [ Set Kernel Arguments ]
    clcheck(m_queue.enqueueWriteBuffer(camera_buff,CL_TRUE,0,sizeof(Camera),camera),
            "camera_buff write");
    
    clcheck(m_queue.enqueueWriteBuffer(seed_memory_buff, CL_TRUE, 0, sizeof(int)*width*height, seed_memory),
            "seed_memory_buff write");
    
    clcheck(m_queue.enqueueWriteBuffer(prim_buff,CL_TRUE,0,sizeof(Primitive)*n_primitives, primitives),
            "prim_buff write");

    clcheck(m_queue.enqueueWriteBuffer(mat_buff, CL_TRUE, 0, sizeof(Material)*n_materials, materials),
            "mat_memory_buff write");


    // new information to be stored
    clcheck(m_queue.enqueueWriteBuffer(color_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, color),
            "color_buff write");
    clcheck(m_queue.enqueueWriteBuffer(origin_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, origin),
            "origin_buff write");
    clcheck(m_queue.enqueueWriteBuffer(direction_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, direction),
            "direction_buff write");
    clcheck(m_queue.enqueueWriteBuffer(weight_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, weight),
            "weight_buff write");
    
    // Global
    int k = 0;
    clcheck(initial_ray_kernel.setArg(k++, m_tri_texture),"Triangles on Texture Buffer");
    clcheck(initial_ray_kernel.setArg(k++, m_bvh_bbox),"BVH on Texture Buffer");
    clcheck(initial_ray_kernel.setArg(k++, m_bvh_info),"BVH on Texture Buffer");
    
    clcheck(initial_ray_kernel.setArg(k++,prim_buff),"Global Primitives");
    clcheck(initial_ray_kernel.setArg(k++,mat_buff),"Global Materials");
    
    clcheck(initial_ray_kernel.setArg(k++,seed_memory_buff),"Global Seed Memory");
    clcheck(initial_ray_kernel.setArg(k++,camera_buff),"Global Camera");
    
    clcheck(initial_ray_kernel.setArg(k++,color_buff),"Global Color");
    clcheck(initial_ray_kernel.setArg(k++,origin_buff),"Global Origin");
    clcheck(initial_ray_kernel.setArg(k++,direction_buff),"Global Direction");
    clcheck(initial_ray_kernel.setArg(k++,weight_buff),"Global Weight");
    
    // Locals
    clcheck(initial_ray_kernel.setArg(k++,local_prim_buff),"Local Primitives");
    clcheck(initial_ray_kernel.setArg(k++,local_mat_buff),"Local Materials");
    
    // Scalars
    clcheck(initial_ray_kernel.setArg(k++,width),"set width");
    clcheck(initial_ray_kernel.setArg(k++,height),"set height");
    clcheck(initial_ray_kernel.setArg(k++,iterations),"set iterations");
    clcheck(initial_ray_kernel.setArg(k++,n_triangles),"set n_triangles");
    clcheck(initial_ray_kernel.setArg(k++,n_materials),"set n_materials");
    clcheck(initial_ray_kernel.setArg(k++,n_primitives),"set n_primitives");
    clcheck(initial_ray_kernel.setArg(k++,n_nodes),"set n_nodes"); //bvh
    
    
    // offset, global work size, local work size
    clcheck(m_queue.enqueueNDRangeKernel(initial_ray_kernel,
                                         cl::NullRange,
                                         cl::NDRange(m_global_size),
                                         cl::NDRange(m_work_item_size)),
            "enqueueNDRangeKernel");
    
    // blocks program until done
    m_queue.finish();
    
    m_queue.enqueueReadBuffer(seed_memory_buff,CL_TRUE,0,sizeof(int)*width*height,seed_memory);
    m_queue.enqueueReadBuffer(color_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,color);
    m_queue.enqueueReadBuffer(origin_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,origin);
    m_queue.enqueueReadBuffer(direction_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,direction);
    m_queue.enqueueReadBuffer(weight_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,weight);
    
    return 0;
}

int OpenCL_Manager::ray_kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                                       int width, int height, int* seed_memory, int iterations, Triangle *triangles,
                                       int n_triangles, Material *materials, int n_materials, int n_nodes,
                                       cl_float4 *color, cl_float4 *origin, cl_float4 *direction,
                                       cl_float4 *weight, int *bounce, cl_float4 *first_color, cl_float4 *first_origin,
                                       cl_float4 *first_direction, cl_float4 *first_weight, int *iteration_memory) {
    
    // [ Global ]
    cl::Buffer prim_buff(m_context,CL_MEM_READ_WRITE,sizeof(Primitive)*n_primitives);
    cl::Buffer camera_buff(m_context,CL_MEM_READ_ONLY,sizeof(Camera));
    cl::Buffer image_buff(m_context,CL_MEM_WRITE_ONLY,sizeof(Pixel)*width*height);
    cl::Buffer mat_buff(m_context,CL_MEM_READ_ONLY,sizeof(Material)*n_materials);


    cl::Buffer seed_memory_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);
    cl::Buffer iteration_memory_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);
    cl::Buffer bounce_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);
    
    cl::Buffer first_origin_buff(m_context,CL_MEM_READ_ONLY,sizeof(cl_float4)*width*height);
    cl::Buffer first_direction_buff(m_context,CL_MEM_READ_ONLY,sizeof(cl_float4)*width*height);
    cl::Buffer first_weight_buff(m_context,CL_MEM_READ_ONLY,sizeof(cl_float4)*width*height);
    cl::Buffer first_color_buff(m_context,CL_MEM_READ_ONLY,sizeof(cl_float4)*width*height);
    
    cl::Buffer origin_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer direction_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer weight_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    cl::Buffer color_buff(m_context,CL_MEM_READ_WRITE,sizeof(cl_float4)*width*height);
    
    
    // [ Local ]
    cl::LocalSpaceArg local_prim_buff = cl::Local(sizeof(Primitive)*n_primitives);
    cl::LocalSpaceArg local_mat_buff = cl::Local(sizeof(Material)*n_materials);
    
    // [ Command Queue ]
    m_queue = cl::CommandQueue(m_context,m_device);
    
    // [ Set Kernel Arguments ]
    clcheck(m_queue.enqueueWriteBuffer(prim_buff,CL_TRUE,0,sizeof(Primitive)*n_primitives, primitives),
            "prim_buff write");
    clcheck(m_queue.enqueueWriteBuffer(camera_buff,CL_TRUE,0,sizeof(Camera),camera),
            "camera_buff write");
    clcheck(m_queue.enqueueWriteBuffer(mat_buff, CL_TRUE, 0, sizeof(Material)*n_materials, materials),
            "mat_memory_buff write");
    
    clcheck(m_queue.enqueueWriteBuffer(seed_memory_buff, CL_TRUE, 0, sizeof(int)*width*height, seed_memory),
            "seed_memory_buff write");
    clcheck(m_queue.enqueueWriteBuffer(iteration_memory_buff, CL_TRUE, 0, sizeof(int)*width*height, iteration_memory),
            "seed_memory_buff write");
    clcheck(m_queue.enqueueWriteBuffer(bounce_buff, CL_TRUE, 0, sizeof(int)*width*height, bounce),
            "bounce_buff write");

    
    // [ current states ]
    clcheck(m_queue.enqueueWriteBuffer(color_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, color),
            "color_buff write");
    clcheck(m_queue.enqueueWriteBuffer(origin_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, origin),
            "origin_buff write");
    clcheck(m_queue.enqueueWriteBuffer(direction_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, direction),
            "direction_buff write");
    clcheck(m_queue.enqueueWriteBuffer(weight_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, weight),
            "weight_buff write");
    
    
    // [ original states ]
    clcheck(m_queue.enqueueWriteBuffer(first_color_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, color),
            "first_color_buff write");
    clcheck(m_queue.enqueueWriteBuffer(first_origin_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, origin),
            "first_origin_buff write");
    clcheck(m_queue.enqueueWriteBuffer(first_direction_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, direction),
            "first_direction_buff write");
    clcheck(m_queue.enqueueWriteBuffer(first_weight_buff, CL_TRUE, 0, sizeof(cl_float4)*width*height, weight),
            "first_weight_buff write");
    
    //__read_only image1d_t texture_triangles,
    //__read_only image1d_t texture_bvh_bbox,
    //__read_only image1d_t texture_bvh_info,
    //
    //const __global float4* global_primitives,
    //const __global float4* global_materials,
    //
    //__global int* seed_memory,
    //__global Camera* camera,
    
//    __global float4 *first_color,
//    __global float4 *first_origin,
//    __global float4 *first_direction,
//    __global float4 *first_weight,
    
    //__global float4 *color,
    //__global float4 *origin,
    //__global float4 *direction,
    //__global float4 *weight,
    //__global int *bounce,
    //
    //__local float4* local_primitives,
    //__local float4* local_materials,
    //
    //int width,
    //int height,
    //int iteration,
    //int n_triangles,
    //int n_materials,
    //int n_primitives,
    //int n_nodes) {
    
    // Global
    int k = 0; // mini-hack
    clcheck(ray_kernel.setArg(k++,m_tri_texture),"Triangles on Texture Buffer");
    clcheck(ray_kernel.setArg(k++, m_bvh_bbox),"BVH on Texture Buffer");
    clcheck(ray_kernel.setArg(k++, m_bvh_info),"BVH on Texture Buffer");
    
    clcheck(ray_kernel.setArg(k++,prim_buff),"Global Primitives");
    clcheck(ray_kernel.setArg(k++,mat_buff),"Global Materials");
    
    clcheck(ray_kernel.setArg(k++,seed_memory_buff),"Global Seed Memory");
    clcheck(ray_kernel.setArg(k++,iteration_memory_buff),"Global Seed Memory");
    clcheck(ray_kernel.setArg(k++,camera_buff),"Global Camera");
    
    clcheck(ray_kernel.setArg(k++,first_color_buff),"Global Color");
    clcheck(ray_kernel.setArg(k++,first_origin_buff),"Global Origin");
    clcheck(ray_kernel.setArg(k++,first_direction_buff),"Global Direction");
    clcheck(ray_kernel.setArg(k++,first_weight_buff),"Global Weight");
    
    clcheck(ray_kernel.setArg(k++,color_buff),"Global Color");
    clcheck(ray_kernel.setArg(k++,origin_buff),"Global Origin");
    clcheck(ray_kernel.setArg(k++,direction_buff),"Global Direction");
    clcheck(ray_kernel.setArg(k++,weight_buff),"Global Weight");
    clcheck(ray_kernel.setArg(k++,bounce_buff),"Global Bounce");
    
    
    // Locals
    clcheck(ray_kernel.setArg(k++,local_prim_buff),"Local Primitives");
    clcheck(ray_kernel.setArg(k++,local_mat_buff),"Local Materials");
    
    // Scalars
    clcheck(ray_kernel.setArg(k++,width),"set width");
    clcheck(ray_kernel.setArg(k++,height),"set height");
    clcheck(ray_kernel.setArg(k++,iterations),"set iterations");
    clcheck(ray_kernel.setArg(k++,n_triangles),"set n_triangles");
    clcheck(ray_kernel.setArg(k++,n_materials),"set n_materials");
    clcheck(ray_kernel.setArg(k++,n_primitives),"set n_primitives");
    clcheck(ray_kernel.setArg(k++,n_nodes),"set n_nodes"); //bvh
    
    
    // offset, global work size, local work size
    clcheck(m_queue.enqueueNDRangeKernel(ray_kernel,
                                         cl::NullRange,
                                         cl::NDRange(m_global_size),
                                         cl::NDRange(m_work_item_size)),
            "enqueueNDRangeKernel");
    
    // blocks program until done
    m_queue.finish();
    
    // [ read int ]
    m_queue.enqueueReadBuffer(seed_memory_buff,CL_TRUE,0,sizeof(int)*width*height,seed_memory);
    m_queue.enqueueReadBuffer(iteration_memory_buff,CL_TRUE,0,sizeof(int)*width*height,iteration_memory);
    m_queue.enqueueReadBuffer(bounce_buff,CL_TRUE,0,sizeof(int)*width*height,bounce);
    
    // [ read float4 ]
    m_queue.enqueueReadBuffer(color_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,color);
    m_queue.enqueueReadBuffer(origin_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,origin);
    m_queue.enqueueReadBuffer(direction_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,direction);
    m_queue.enqueueReadBuffer(weight_buff,CL_TRUE,0,sizeof(cl_float4)*width*height,weight);
    
    return 0;
}

//__kernel void reconstruct_kernel(__global Pixel *image,
//                                 __global float4 *contribution,
//                                 int iterations)

int OpenCL_Manager::reconstruct_kernel_execute(Pixel *image, int dim, cl_float4 *contribution,
                                               int* iteration_memory) {

    cl::Buffer image_buff(m_context,CL_MEM_READ_WRITE,sizeof(Pixel)*dim);
    cl::Buffer contribution_buff(m_context,CL_MEM_READ_ONLY,sizeof(cl_float4)*dim);
    cl::Buffer iteration_memory_buff(m_context,CL_MEM_READ_ONLY,sizeof(int)*dim);
    
    clcheck(m_queue.enqueueWriteBuffer(image_buff, CL_TRUE, 0, sizeof(Pixel)*dim, image),
            "image_buff write");
    clcheck(m_queue.enqueueWriteBuffer(contribution_buff, CL_TRUE, 0, sizeof(cl_float4)*dim, contribution),
            "contribution_buff write");
    clcheck(m_queue.enqueueWriteBuffer(iteration_memory_buff, CL_TRUE, 0, sizeof(int)*dim, iteration_memory),
            "iteration_memory_buff write");
    
    // [ Command Queue ]
    m_queue = cl::CommandQueue(m_context,m_device);
    
    //
    int k = 0;
    clcheck(reconstruct_kernel.setArg(k++, image_buff),"image buffer");
    clcheck(reconstruct_kernel.setArg(k++, contribution_buff),"contribution buffer");
    clcheck(reconstruct_kernel.setArg(k++, iteration_memory_buff),"iteration");
    
    //
    clcheck(m_queue.enqueueNDRangeKernel(reconstruct_kernel,
                                         cl::NullRange,
                                         cl::NDRange(m_global_size),
                                         cl::NDRange(m_work_item_size)),
            "enqueueNDRangeKernel");

    // blocks program until done
    m_queue.finish();
    m_queue.enqueueReadBuffer(image_buff,CL_TRUE,0,sizeof(Pixel)*dim,image);
    
    return 1;
}

int OpenCL_Manager::kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                                   Pixel *image, int width, int height, int* seed_memory,
                                   int iterations, Triangle *triangles, int n_triangles,
                                   Material *materials, int n_materials,
                                   BVH_Node *nodes, int n_nodes) {
    
    // [ Global ]
    cl::Buffer prim_buff(m_context,CL_MEM_READ_WRITE,sizeof(Primitive)*n_primitives);
    cl::Buffer camera_buff(m_context,CL_MEM_READ_ONLY,sizeof(Camera));
    cl::Buffer image_buff(m_context,CL_MEM_WRITE_ONLY,sizeof(Pixel)*width*height);
    cl::Buffer seed_memory_buff(m_context,CL_MEM_READ_WRITE,sizeof(int)*width*height);

    cl::Buffer mat_buff(m_context,CL_MEM_READ_ONLY,sizeof(Material)*n_materials);
    cl::Buffer bvh_buff(m_context,CL_MEM_READ_ONLY,sizeof(BVH_Node)*n_nodes);
    
    // [ Local ]
    cl::LocalSpaceArg local_prim_buff = cl::Local(sizeof(Primitive)*n_primitives);
    cl::LocalSpaceArg local_mat_buff = cl::Local(sizeof(Material)*n_materials);
    cl::LocalSpaceArg local_bvh_buff = cl::Local(sizeof(BVH_Node)*n_nodes);
    
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
    clcheck(m_queue.enqueueWriteBuffer(mat_buff, CL_TRUE, 0, sizeof(Material)*n_materials, materials),
            "mat_memory_buff write");
    clcheck(m_queue.enqueueWriteBuffer(bvh_buff, CL_TRUE, 0, sizeof(BVH_Node)*n_nodes, nodes),
            "bvh_node_buff write");

    
    // Global
    int k = 0; // mini-hack
    clcheck(m_kernel.setArg(k++,m_tri_texture),"Triangles on Texture Buffer");
    clcheck(m_kernel.setArg(k++, m_bvh_bbox),"BVH on Texture Buffer");
    clcheck(m_kernel.setArg(k++, m_bvh_info),"BVH on Texture Buffer");
    
    clcheck(m_kernel.setArg(k++,prim_buff),"Global Primitives");
    clcheck(m_kernel.setArg(k++,mat_buff),"Global Materials");

    clcheck(m_kernel.setArg(k++,image_buff),"Global Image");
    clcheck(m_kernel.setArg(k++,seed_memory_buff),"Global Seed Memory");
    clcheck(m_kernel.setArg(k++,camera_buff),"Global Camera");
    
    // Locals
    clcheck(m_kernel.setArg(k++,local_prim_buff),"Local Primitives");
    clcheck(m_kernel.setArg(k++,local_mat_buff),"Local Materials");
    
    // Scalars
    clcheck(m_kernel.setArg(k++,width),"set width");
    clcheck(m_kernel.setArg(k++,height),"set height");
    clcheck(m_kernel.setArg(k++,iterations),"set iterations");
    clcheck(m_kernel.setArg(k++,n_triangles),"set n_triangles");
    clcheck(m_kernel.setArg(k++,n_materials),"set n_materials");
    clcheck(m_kernel.setArg(k++,n_primitives),"set n_primitives");
    clcheck(m_kernel.setArg(k++,n_nodes),"set n_nodes"); //bvh
    
    
    // offset, global work size, local work size
    clcheck(m_queue.enqueueNDRangeKernel(m_kernel,
                                      cl::NullRange,
//                                     cl::NDRange(1),
//                                      cl::NullRange),
                                      cl::NDRange(m_global_size),
                                      cl::NDRange(m_work_item_size)),
//                                      cl::NDRange(m_work_item_size)),
            
            "enqueueNDRangeKernel");

    // blocks program until done
    m_queue.finish();
    m_queue.enqueueReadBuffer(image_buff,CL_TRUE,0,sizeof(Pixel)*width*height,image);
    m_queue.enqueueReadBuffer(seed_memory_buff,CL_TRUE,0,sizeof(int)*width*height,seed_memory);
    
//    std::vector<size_t> wgs = std::vector<size_t>(3);
//    m_kernel.getWorkGroupInfo<std::vector<size_t> >(m_device,CL_KERNEL_WORK_GROUP_SIZE,&wgs);
//    
//    cl_ulong lms;
//    m_kernel.getWorkGroupInfo<cl_ulong>(m_device, CL_KERNEL_LOCAL_MEM_SIZE, &lms);
//    
//    cl_ulong dlms;
//    m_device.getInfo<cl_ulong>(CL_DEVICE_LOCAL_MEM_SIZE, &dlms);
//    
//    //        CL_KERNEL_LOCAL_MEM_SIZE
//    //        CL_DEVICE_LOCAL_MEM_SIZE
//    
//    std::cout << "device local mem size " << dlms << "\n";
//
//    std::cout << "local memory size " << sizeof(BVH_Node) * n_nodes
//    + sizeof(Material) * n_materials + sizeof(Primitive) * n_primitives << "\n";
    
    return 0;
}