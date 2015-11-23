#ifndef __pathtracer__pathtracer__
#define __pathtracer__pathtracer__

#include <stdio.h>
#include <stdlib.h> // rand()
#include "common.hpp"
#include "opencl_manager.hpp"

class Pathtracer {

private:
    int n_primitives;
    
    Primitive *primitives;
    Camera *camera;
    OpenCL_Manager *ocl_manager;
    
    Pixel* image_data;
    int* seed_memory;
    
    int width;
    int height;
    
    int iterations;

public:
    Pathtracer();

    // setters
    // use fake scene for now
    int set_scene();
    int set_camera();
    Pixel* fake_render();
    
    // getters
    int get_width();
    int get_height();
};

#endif
