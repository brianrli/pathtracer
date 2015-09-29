#include "pathtracer.hpp"

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
    n_primitives = 1;

    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    set(primitives[0].center,Vec4f());
    primitives[0].radius = 0.5f;
    return 1;
}

// set camera
int Pathtracer::set_camera() {
    camera = new Camera();
    set(camera->eye,Vec4f());
    set(camera->n,Vec4f(0,0,-1,0));
    set(camera->u,Vec4f(1,0,0,0));
    set(camera->v,Vec4f(0,1,0,0));
    return 1;
}

// fake render
Bitmap* Pathtracer::fake_render() {

    Bitmap *bmp = new Bitmap();

    // testing
    int width = 72;
    int height = 48;

    Pixel *image = new Pixel[width*height];
    
    // path trace
    ocl_manager->kernel_execute(primitives,camera,n_primitives,
        image,width,height);

    bmp->pixels = image;
    bmp->width = width;
    bmp->height = height;
    
    return bmp;
    // Camera camera = Camera();
    
    // for (int i = 0; i < width; i++) {
    //     for (int j = 0; j < height; j++) {
            
//            Vec4f origin = camera.position;
//            
//            float x = (float)i/width - 0.5f;
//            float y = (float)j/height - 0.5f;
//            
//            Vec4f direction = origin + (x * camera.u) + (y * camera.v);
//            direction.print();
        // }
//    }
};
