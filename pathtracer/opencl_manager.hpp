#ifndef pathtracer_pathtracer_opencl_h
#define pathtracer_pathtracer_opencl_h

#include <Opencl/cl.hpp>
#include "common.hpp"

class OpenCL_Manager {
    
public:
    // Constructor
    OpenCL_Manager();

    int initialize(int width, int height);
    
    int kernel_load(std::string file_name,
                    std::string kernel_name);
    
    int new_kernel_load(int kid, std::string file_name,
                        std::string kernel_name);

    int triangles_to_texture(Triangle *triangles,int n_triangles);
    int bvh_to_texture(BVH_Node_BBox *bbox,
                       BVH_Node_Info *info,
                       int n_nodes);
    
    // Execute Kernel
    int kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                       Pixel *image, int width, int height, int* seed_memory,
                       int iterations, Triangle *triangles, int n_triangles,
                       Material *materials, int n_materials,
                       BVH_Node *nodes, int n_nodes);
    
    // Iterative Ray Kernel
    int ray_kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                           int width, int height, int* seed_memory, int iterations, Triangle *triangles,
                           int n_triangles, Material *materials, int n_materials, int n_nodes,
                           cl_float4 *color, cl_float4 *origin, cl_float4 *direction,
                           cl_float4 *weight, int *bounce, cl_float4 *first_color, cl_float4 *first_origin,
                           cl_float4 *first_direction, cl_float4 *first_weight, int *iteration_memory);
    
    // Initial Ray Kernel
    int initial_ray_kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                                  int width, int height, int* seed_memory,
                                  int iterations, Triangle *triangles, int n_triangles,
                                  Material *materials, int n_materials,
                                  BVH_Node *nodes, int n_nodes, cl_float4 *color,
                                  cl_float4 *origin, cl_float4 *direction, cl_float4 *weight);
    
    // Reconstruct Kernel
    int reconstruct_kernel_execute(Pixel *image, int dim, cl_float4 *contribution, int* iteration_memory);
    
private:
    int m_global_size;
    int m_work_item_size;
    
    cl::Platform m_platform;
    cl::Device m_device;
    cl::Context m_context;
    cl::Program m_program;
    cl::CommandQueue m_queue;
    
    cl::Kernel m_kernel;
    
    // new
    cl::Kernel reconstruct_kernel;
    cl::Kernel ray_kernel;
    cl::Kernel initial_ray_kernel;

    cl::Image1D m_tri_texture;

    cl::Image1D m_bvh_texture;
    cl::Image1D m_bvh_bbox;
    cl::Image1D m_bvh_info;
    
    std::vector<cl::Buffer> m_buffers;
};

#endif
