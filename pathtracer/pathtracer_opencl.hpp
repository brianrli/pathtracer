#ifndef pathtracer_pathtracer_opencl_h
#define pathtracer_pathtracer_opencl_h

#include <Opencl/cl.hpp>

typedef unsigned char Color;

struct Pixel {
    Color red;
    Color green;
    uint8_t blue;
};

struct Bitmap {
    Pixel *pixels;
    int width;
    int height;
};

struct Primitive {
    cl_float4 center;
    cl_float origin;
    cl_float3 padding;
};

class OpenCL_Manager {
    
public:
    OpenCL_Manager();
    int initialize();
    int load_kernel(std::string file_name,
                    std::string kernel_name);
    int execute_kernel();
    
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
