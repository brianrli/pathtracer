#ifndef pathtracer_pathtracer_opencl_h
#define pathtracer_pathtracer_opencl_h

#include <Opencl/cl.hpp>
#include "common.hpp"

class OpenCL_Manager {
    
public:
    // constructor
    OpenCL_Manager();

    int initialize();
    
    int kernel_load(std::string file_name,
                    std::string kernel_name);
    
    int kernel_execute(Primitive *primitives, Camera *camera, int n_primitives,
                       Pixel *image, int width, int height);
    
private:
    cl::Platform m_platform;
    cl::Device m_device;
    cl::Context m_context;
    cl::Program m_program;
    cl::CommandQueue m_queue;

    cl::Kernel m_kernel;
    
    std::vector<cl::Buffer> m_buffers;
};

#endif
