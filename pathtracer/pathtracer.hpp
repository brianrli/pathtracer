#ifndef __pathtracer__pathtracer__
#define __pathtracer__pathtracer__

#include <stdio.h>
#include "common.hpp"
#include "opencl_manager.hpp"

class Pathtracer {

private:
    int n_primitives;
    Primitive *primitives;
    Camera *camera;
    OpenCL_Manager *ocl_manager;

public:
    Pathtracer();

    // use fake scene for now
    int set_scene();
    int set_camera();
    Bitmap* fake_render();
};

#endif 
/* defined(__pathtracer__pathtracer__) */
