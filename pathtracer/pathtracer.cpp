#include "pathtracer.hpp"

// Primitive Constructor
Primitive create_primitive(){
    Primitive primitive;

    // material
    primitive.refractive = (cl_float4){1.0f,0,0,0};
    primitive.specular = (cl_float4){0,0,0,0};
    primitive.diffuse = (cl_float4){0,0,0,0};

    // emissive.w is 1 if light
    primitive.emissive = (cl_float4){0,0,0,0};
    
    primitive.center = (cl_float4){0,0,0,0};
    
    primitive.radius = 1.0f;
    return primitive;
}

// Material Constructor
Material create_material() {
    Material material;
    material.refractive = (cl_float4){1.0f,0,0,0};
    material.specular = (cl_float4){0,0,0,0};
    material.diffuse = (cl_float4){0,0,0,0};
    material.emissive = (cl_float4){0,0,0,0};
    return material;
}

// Triangle Constructor
Triangle create_triangle() {
    Triangle triangle;
    triangle.v1 = (cl_float3){0,0,0};
    triangle.v2 = (cl_float3){0,0,0};
    triangle.v3 = (cl_float3){0,0,0};
    triangle.material = (cl_float3){0,0,0};
    return triangle;
}

Triangle create_triangle(Vec3f v1, Vec3f v2, Vec3f v3)
{
    Triangle triangle;
    
    triangle.v1 = (cl_float3){v1[0],v1[1],v1[2]};
    triangle.v2 = (cl_float3){v2[0],v2[1],v2[2]};
    triangle.v3 = (cl_float3){v3[0],v3[1],v3[2]};
    triangle.material = (cl_float3){0,0,0};

    return triangle;
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
    
    // [ Path Trace !! ]
    ocl_manager->kernel_execute(primitives,camera,n_primitives,
                                image,width,height,seed_memory,iterations,
                                triangles,n_triangles,materials,n_materials);
    
    iterations++;
    
    
    // [ Accumulate Contributions ]
    if(iterations > 1) {
        //new
        float i1 = 1/(float)iterations;
        float i2 = (float)(iterations-1)/(float)iterations;
        
        for (int i = 0; i < width*height; i++) {
            image_data[i].red = (i1*image[i].red)+(i2*image_data[i].red);
            image_data[i].blue = (i1*image[i].blue)+(i2*image_data[i].blue);
            image_data[i].green = (i1*image[i].green)+(i2*image_data[i].green);
        }
        
    }
    else if (iterations == 1){

        for (int i = 0; i < width*height; i++) {
            image_data[i] = image[i];
        }
    }
    
    delete [] image;
    return image_data;
};

// set camera
int Pathtracer::set_camera() {
    camera = new Camera();
    // defaults for now
    Vec4f lookat = Vec4f();
    Vec4f eye = Vec4f(0,0,-3,0);
    
    set(camera->eye,Vec4f(0,0,-6,0));
    set(camera->n,Vec4f(0,0,-1,0));
    set(camera->u,Vec4f(1,0,0,0));
    set(camera->v,Vec4f(0,1,0,0));
    return 1;
}

void printclf3(cl_float3 f) {
    printf("%f %f %f   ",f.x,f.y,f.z);
}

int Pathtracer::set_triangles(std::vector<Vec3f> vertices) {
    
    // has to be divisible by three
    if ((vertices.size() % 3) != 0 ){
        return 0;
    }
    
    // [ Triangle ]
    n_triangles = vertices.size() / 3;
    triangles = (Triangle*)malloc(sizeof(Triangle) * n_triangles);
    
    int index = 0;
    
    for (int i = 0; i < vertices.size(); i += 3) {
        triangles[index] = create_triangle(vertices[i],vertices[i+1],
                                       vertices[i+2]);
        index++;
    }
    
//    for (int d = 0; d < n_triangles; d++) {
//        printclf3(triangles[d].v1);
//        printclf3(triangles[d].v2);
//        printclf3(triangles[d].v3);
//        printf("\n");
//    }
    
    return 1;
}

// set up (amazing) fake scene
int Pathtracer::set_scene() {
    
    // [ Material ]
    n_materials = 1;
    materials = (Material*)malloc(sizeof(Material) * n_materials);
    materials[0] = create_material();
    materials[0].diffuse = {0.7,0.2,0.2,0.0};
    
    // [ Primitives ]
    n_primitives = 1;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
//    // ground plane
//    primitives[0] = create_primitive();
//    primitives[0].type = {1,0,0};
//    primitives[0].center = {0,0.5,0,0};
//    primitives[0].plane_normal = {0,-1,0,0};
//    primitives[0].diffuse = {0.4,0.4,0.4,0};
//    
    // create light
    primitives[0] = create_primitive();
    primitives[0].center = (cl_float4){0.8,-0.7,0,0};
    primitives[0].type = {0,0,0};
    primitives[0].radius = 0.2f;
    primitives[0].emissive = (cl_float4){10,10,10,10};
    
//    // reflective sphere
//    primitives[2] = create_primitive();
//    primitives[2].type = {0,0,0};
//    primitives[2].center = {0.5,0.2,0,0};
//    primitives[2].radius = .3f;
//    primitives[2].refractive = {1.0,0,0,0};
//    primitives[2].specular = {0.2,0.7,0.2,0};
//    primitives[2].diffuse = {0.2,0.7,0.2,0};
//    
//    // refractive sphere
//    primitives[3] = create_primitive();
//    primitives[3].type = {0,0,0};
//    primitives[3].center = {0.1,0.2,0.0f,0};
//    primitives[3].radius = .3f;
//    primitives[3].refractive = {1.1,0,0,1};
    
    return 1;
}