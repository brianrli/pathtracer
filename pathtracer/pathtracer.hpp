#ifndef __pathtracer__pathtracer__
#define __pathtracer__pathtracer__

#include <stdio.h>
#include <stdlib.h> // rand()
#include <vector>
#include "common.hpp"
#include "opencl_manager.hpp"
#include "bvh.hpp"

class Pathtracer {

private:
    // build bvh
    void construct_bvh();

    Primitive *primitives;
    Triangle *triangles;
    Material *materials;
    Camera *camera;
    OpenCL_Manager *ocl_manager;
    BVH_Node *bvh_nodes;
    BVH_Node_Info *bvh_info;
    BVH_Node_BBox *bvh_bbox;
    
    Pixel* image_data;
    int* seed_memory;
    
    // [new memory]
    cl_float4 *first_bounce_origin;
    cl_float4 *first_bounce_direction;
    cl_float4 *first_bounce_color;
    cl_float4 *first_bounce_weight;

    cl_float4 *origin;
    cl_float4 *direction;
    
    cl_float4 *weight;
    cl_float4 *color;
    
    int *iteration_memory;
    int *bounce;
    
    int n_primitives;
    int n_materials;
    int n_triangles;
    int n_nodes;
    int width;
    int height;
    int iterations;
    
    BoundingVolumeHierarchy bvh_constructor;
    std::vector<VectorTriangle> vector_triangles;
public:
    Pathtracer(int w, int h);

    // setters
    // use fake scene for now
    int set_scene();
    int set_camera();
    int set_triangles(std::vector<Vec3f> vertices);

    Pixel* render();
    
    //iterative render
    void iterative_render_first_bounce();
    
    void iterative_render();
    
    void reconstruct();
    
    // getters
    int get_width();
    int get_height();
    int get_iterations();
    Pixel* get_image();
};

#endif
