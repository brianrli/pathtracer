#ifndef __pathtracer__pathtracer__
#define __pathtracer__pathtracer__

#include <stdio.h>
#include <stdlib.h> // rand()
#include <vector>
#include "common.hpp"
#include "opencl_manager.hpp"

class Pathtracer {

private:
    Primitive *primitives;
    Triangle *triangles;
    Material *materials;
    Camera *camera;
    OpenCL_Manager *ocl_manager;
    Pixel* image_data;
    int* seed_memory;
    
    int n_primitives;
    int n_materials;
    int n_triangles;
    int width;
    int height;
    int iterations;

public:
    Pathtracer();

    // setters
    // use fake scene for now
    int set_scene();
    int set_camera();
    int set_triangles(std::vector<Vec3f> vertices);
    Pixel* fake_render();
    
    // getters
    int get_width();
    int get_height();
};

#endif
