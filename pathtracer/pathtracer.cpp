#include "pathtracer.hpp"

// primitive constructor
Primitive create_primitive(){
    Primitive primitive;

    // material
    primitive.refractive = (cl_float4){1.0f,0,0,0};
    primitive.reflective = (cl_float4){0,0,0,0};
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
    
    width = 720;
    height = 720;
    
    seed_memory = new int[width*height*2];
    for(int i = 0; i < (width*height); i++) {
        seed_memory[i] = (rand() % 2147483646)+1;
    }
}

//getters
int Pathtracer::get_width() {
    return width;
}

int Pathtracer::get_height() {
    return height;
}

// fake render
Pixel* Pathtracer::fake_render() {
    
    // testing
    Pixel *image = new Pixel[width*height];
    
    // path trace
    ocl_manager->kernel_execute(primitives,camera,n_primitives,
                                image,width,height,seed_memory);

    return image;
};

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

// set up (amazing) fake scene
int Pathtracer::set_scene() {
    
    // what is going on with the Y
    n_primitives = 5;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
    // create sphere
    primitives[0] = create_primitive();
    primitives[0].type = {3,0,0}; // doesn't do anything
    primitives[0].center = {0,0,3,0};
    primitives[0].radius = 0.1f;
    primitives[0].diffuse = {0.2,0.8,0.5,0};
    primitives[0].reflective = {0,0,0,1};
    
    // second sphere
    primitives[1] = create_primitive();
    primitives[1].type = {0,0,0};
    primitives[1].center = {0,0,0,0};
    primitives[1].radius = .5f;
    primitives[1].diffuse = {1.0,0.3,0.3,0};
    //    primitives[1].refractive = {1.2f,0.5f,0.0f,1.0f};
    //    primitives[1].specular = {0.01,0.02,0.01,2.0};
    
    
    // create light
    primitives[2] = create_primitive();
    primitives[2].center = (cl_float4){0,-4,0,0};
    primitives[2].emissive.s0 = 1;
    primitives[2].emissive.s3 = 1;
    
    // plane
    primitives[3] = create_primitive();
    primitives[3].type = {1,0,0};
    primitives[3].center = (cl_float4){0,0,3,0};
    primitives[3].plane_normal = (cl_float4){0,0,-1,0};
    primitives[3].diffuse = {0.7,0.2,0.2,0}; // treat as emissive
    
    primitives[4] = create_primitive();
    primitives[4].type = {1,0,0};
    primitives[4].center = (cl_float4){0,0.5,0,0};
    primitives[4].plane_normal = (cl_float4){0,-1,0,0};
    primitives[4].diffuse = {0.2,0.2,0.7,0}; // treat as emissive
    //    primitives[4].specular = {0.2,0.2,0.2,0}; //
    
    return 1;
}