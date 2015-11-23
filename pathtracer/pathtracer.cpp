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
    
    iterations = 0;
    
    seed_memory = new int[width*height*2];
    for(int i = 0; i < (width*height); i++) {
        seed_memory[i] = (rand() % 2147483646)+1;
    }
    
    image_data = new Pixel[width*height];
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
    
    iterations++;
    
    
    // average paths
    if(iterations > 1) {
        //new
        float i1 = 1/(float)iterations;
        float i2 = (float)(iterations-1)/(float)iterations;

    
//        std::cout << i1 << " " << i2 << std::endl;
        for (int i = 0; i < width*height; i++) {
            image_data[i].red = (i1*image[i].red)+(i2*image_data[i].red);
            image_data[i].blue = (i1*image[i].blue)+(i2*image_data[i].blue);
            image_data[i].green = (i1*image[i].green)+(i2*image_data[i].green);
        }
        
        delete [] image;
    }else if (iterations == 1){
        for (int i = 0; i < width*height; i++) {
            image_data[i] = image[i];
        }
        
        delete [] image;
    }
    else {
        delete [] image;
    }
    
    return image_data;
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
    n_primitives = 3;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
    // second sphere
    primitives[0] = create_primitive();
    primitives[0].type = {0,0,0};
    primitives[0].center = {1,0,0,0};
    primitives[0].radius = .5f;
    primitives[0].diffuse = {0.3,0.7,0.7,0};
    
    // create light
    primitives[1] = create_primitive();
    primitives[1].center = (cl_float4){0,-1,0,0};
    primitives[1].type = {0,0,0};
    primitives[1].radius = 0.2f;
    primitives[1].emissive = (cl_float4){35,35,35,35};
    
    // planes
    primitives[2] = create_primitive();
    primitives[2].type = {1,0,0};
    primitives[2].center = {0,2,0,0};
    primitives[2].plane_normal = {0,-1,0,0};
    primitives[2].diffuse = {0.3,0.3,0.3,0};
    
    return 1;
}