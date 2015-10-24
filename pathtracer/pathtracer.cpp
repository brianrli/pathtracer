#include "pathtracer.hpp"

// primitive constructor
Primitive create_primitive(){
    Primitive primitive;
    
    primitive.specular = (cl_float4){0,0,0,0};
    primitive.diffuse = (cl_float4){0,0,0,0};
    // emissive.w is 1 if light
    primitive.emissive = (cl_float4){0,0,0,0};
    
    primitive.center = (cl_float4){0,0,0,0};
    primitive.radius = 1.0f;
    return primitive;
}

void set(cl_float4 &dst, Vec4f vec) {
    for (int i = 0; i < 4; i++) {
        dst.s[i] = vec[i];
    }
}

// constructor
Pathtracer::Pathtracer() {
    ocl_manager = new OpenCL_Manager();
    ocl_manager->initialize();
    ocl_manager->kernel_load("pathtracer_kernel.cl",
                             "pathtracer_kernel");
}

// set up (amazing) fake scene
int Pathtracer::set_scene() {

    n_primitives = 3;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
    // create sphere
    primitives[0] = create_primitive();
    primitives[0].radius = 0.4f;
    primitives[0].diffuse = {0.2,0.8,0.5,0};
    primitives[0].specular = {0.7,0.7,0.7,4.0};

    // second sphere
    primitives[1] = create_primitive();
    primitives[1].center = {1,0,0,0};
    primitives[1].radius = 1.0f;
    primitives[1].diffuse = {0.8,0.2,0.2,0};

    // create light
    primitives[2] = create_primitive();
    primitives[2].center = (cl_float4){-1,-1,-2,0};
    primitives[2].emissive.s0 = 0.8;
    primitives[2].emissive.s3 = 1;
    
    return 1;
}

// set camera
int Pathtracer::set_camera() {
    camera = new Camera();
    // defaults for now
    set(camera->eye,Vec4f(0,0,-3,0));
    set(camera->n,Vec4f(0,0,-1,0));
    set(camera->u,Vec4f(1,0,0,0));
    set(camera->v,Vec4f(0,1,0,0));
    return 1;
}

// fake render
Bitmap* Pathtracer::fake_render() {

    Bitmap *bmp = new Bitmap();

    // testing
    int width = 720;
    int height = 720;

    Pixel *image = new Pixel[width*height];
    
    // path trace
    ocl_manager->kernel_execute(primitives,camera,n_primitives,image,width,height);

    bmp->pixels = image;
    bmp->width = width;
    bmp->height = height;
    
    return bmp;
};
