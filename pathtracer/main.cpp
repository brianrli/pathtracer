//
//  main.cpp
//  opencl_test_ii
//
//  Created by Brian Li on 9/8/15.
//  Copyright (c) 2015 Brian Li. All rights reserved.
//

#include <OpenCL/cl.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

int main(int argc, const char * argv[]) {
    
    // setup platform
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    
    if(all_platforms.size()==0){
        std::cout<<" No platforms found."<<std::endl;
        exit(1);
    }
    
    cl::Platform default_platform=all_platforms[0];
    
    // setup device of type GPU
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if (all_devices.size()==0) {
        std::cout << "No GPU Devices found." << std::endl;
        exit(1);
    }
    
    cl::Device default_device=all_devices[0];
    
    // setup context
    cl::Context context({default_device});
    
    // setup program
    std::ifstream stream("pathtracer_kernel.cl");
    
    if (!stream.is_open()) {
        std::cout << "Cannot open kernel." << std::endl;
        exit(1);
    }
    
    std::string source = std::string(std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>());

    cl::Program program(context,source,true);
    
    if(program.build({default_device})!=CL_SUCCESS) {
        std::cout << "Error building." << std::endl;
        exit(1);
    }
    
    // setup kernel
    cl::Kernel kernel(program,"matvec");

    // setup data & buffers
    float mat[16], vec[4], result[4];

    for (int i=0; i<16; i++) {
        mat[i] = i;
    }

    for (int i=0; i<4; i++) {
        vec[i] = i * 3.0f;
    }

    cl::Buffer mat_buff(context,CL_MEM_READ_WRITE,sizeof(float)*16);
    cl::Buffer vec_buff(context,CL_MEM_READ_WRITE,sizeof(float)*4);
    cl::Buffer res_buff(context,CL_MEM_READ_WRITE,sizeof(float)*4);

    // create command queue
    cl::CommandQueue queue(context,default_device);

    // set kernel arguments
    queue.enqueueWriteBuffer(mat_buff,CL_TRUE,0,sizeof(float)*16,mat);
    queue.enqueueWriteBuffer(vec_buff,CL_TRUE,0,sizeof(float)*4,vec);
    
    kernel.setArg(0,mat_buff);
    kernel.setArg(1,vec_buff);
    kernel.setArg(2,res_buff);
    
    queue.enqueueNDRangeKernel(kernel, // kernel
                               cl::NullRange, // offset
                               cl::NDRange(4), // global work size
                               cl::NullRange); // local work size
    
    // blocks program until done
    queue.finish();
    queue.enqueueReadBuffer(res_buff,CL_TRUE,0,sizeof(float)*4,result);

    // output
    for (int i=0; i<4; i++) {
        std::cout<< "result[" <<i<<"] " << result[i] << std::endl;
    }
    std::cout << std::endl;

    return 0;
}


