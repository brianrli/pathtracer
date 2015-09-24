#include <vector>
#include <iostream>
#include <fstream>
#include "pathtracer_opencl.hpp"


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
    
int OpenCL_Manager::load_kernel(std::string file_name,
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
    
    if(m_program.build({m_device})!=CL_SUCCESS) {
        std::cout << "Error building." << std::endl;
        exit(1);
    }

    // setup kernel
    m_kernel = cl::Kernel(m_program,kernel_name.c_str());
    
    return 0;
}
    

int OpenCL_Manager::execute_kernel() {
    
    // setup data & buffers
    float mat[16], vec[4], result[4];
    
    for (int i=0; i<16; i++) {
        mat[i] = i;
    }
    
    for (int i=0; i<4; i++) {
        vec[i] = i * 3.0f;
    }
    
    cl::Buffer mat_buff(m_context,CL_MEM_READ_WRITE,sizeof(float)*16);
    cl::Buffer vec_buff(m_context,CL_MEM_READ_WRITE,sizeof(float)*4);
    cl::Buffer res_buff(m_context,CL_MEM_READ_WRITE,sizeof(float)*4);
    
    // create command queue
    m_queue = cl::CommandQueue(m_context,m_device);
    
    // set kernel arguments
    m_queue.enqueueWriteBuffer(mat_buff,CL_TRUE,0,sizeof(float)*16,mat);
    m_queue.enqueueWriteBuffer(vec_buff,CL_TRUE,0,sizeof(float)*4,vec);
    
    m_kernel.setArg(0,mat_buff);
    m_kernel.setArg(1,vec_buff);
    m_kernel.setArg(2,res_buff);
    
    m_queue.enqueueNDRangeKernel(m_kernel, // kernel
                               cl::NullRange, // offset
                               cl::NDRange(4), // global work size
                               cl::NullRange); // local work size
    
    // blocks program until done
    m_queue.finish();
    m_queue.enqueueReadBuffer(res_buff,CL_TRUE,0,sizeof(float)*4,result);
        
    return 0;
}