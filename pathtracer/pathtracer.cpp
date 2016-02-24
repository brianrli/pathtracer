#include "pathtracer.hpp"

static float ONE_THIRD_RT = 0.57735026919f;

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

// [ Triangles ]
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
    
    camera = new Camera();
}

//getters
int Pathtracer::get_width() {
    return width;
}

int Pathtracer::get_height() {
    return height;
}

Pixel* Pathtracer::get_image() {
    return image_data;
}

int Pathtracer::get_iterations(){
    return iterations;
}

// fake render
Pixel* Pathtracer::render() {
    
    // [ build bounding volume hierarchy ]
    
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
    
    // defaults for now
//    {0.3,0.7,-0.7,0}
    Vec4f lookat = Vec4f(0,0,0,0);
//    Vec4f eye = Vec4f(0,0,-3,0);
    Vec4f eye = Vec4f(2.0,-0.9,-3,0);

    Vec4f most_perpendicular;
    Vec4f look_vec = lookat - eye;

    
    most_perpendicular = Vec4f(0,1,0,0);

    Vec4f n = -look_vec;
    n.normalize();
    Vec4f u = most_perpendicular^look_vec;
    u.normalize();
    Vec4f v = n^u;
    v.normalize();
//
//    n.print();
//    u.print();
//    v.print();
//    
    set(camera->eye,eye);
    set(camera->n,Vec4f(0,0,-1,0));
    set(camera->u,Vec4f(1,0,0,0));
    set(camera->v,Vec4f(0,1,0,0));
    
    set(camera->n,n);
    set(camera->u,u);
    set(camera->v,-v);
    
    return 1;
}

void printclf3(cl_float3 f) {
    printf("%f %f %f   ",f.x,f.y,f.z);
}

int Pathtracer::set_triangles(std::vector<Vec3f> vertices) {
    
    // debugging purposes
    n_triangles = 2;
    triangles = (Triangle*)malloc(sizeof(Triangle) * n_triangles);

//     has to be divisible by three
    if ((vertices.size() % 3) != 0 ){
        return 0;
    }
    
    // [ Triangle ]
    n_triangles = vertices.size() / 3;
    triangles = (Triangle*)malloc(sizeof(Triangle) * n_triangles);
    
    int index = 0;
    
    // initialize triangles
    for (int i = 0; i < vertices.size(); i += 3) {
        triangles[index] = create_triangle(vertices[i],vertices[i+1],
                                       vertices[i+2]);
        vector_triangles.push_back(VectorTriangle(vertices[i],vertices[i+1],
                                                  vertices[i+2]));
        index++;
    }
    
    return 1;
}

// set up (amazing) fake scene
int Pathtracer::set_scene() {
    
    // [ Material ]
    n_materials = 1;
    materials = (Material*)malloc(sizeof(Material) * n_materials);
    materials[0] = create_material();
    materials[0].diffuse = {0.2,0.6,0.2,0.0};
    
    // [ Primitives ]
    n_primitives = 2;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
//    // ground plane
    primitives[0] = create_primitive();
    primitives[0].type = {1,0,0};
    primitives[0].center = {0,1,0,0};
    primitives[0].plane_normal = {0,-1,0,0};
    primitives[0].diffuse = {0.3,0.3,0.3,0};
//
    // create light
    primitives[1] = create_primitive();
    primitives[1].center = (cl_float4){-0.3,-0.7,0,0};
    primitives[1].type = {0,0,0};
    primitives[1].radius = .2;
    primitives[1].emissive = {10,10,10,10};
    
//    // reflective sphere
//    primitives[2] = create_primitive();
//    primitives[2].type = {0,0,0};
//    primitives[2].center = (cl_float4){0.0,0.4,0,0};
//    primitives[2].radius = .4;
//    primitives[2].specular = {0.7,0.7,0.2,0};
//    primitives[2].refractive = {1.9,0,0,0.0};
//    
//    primitives[3] = create_primitive();
//    primitives[3].type = {0,0,0};
//    primitives[3].center = {0.8,0.7,0.3f,0};
//    primitives[3].radius = .3f;
//    primitives[3].diffuse = {0.8,0.8,0.8,0};
    
//
//    // refractive sphere
//    primitives[3] = create_primitive();
//    primitives[3].type = {0,0,0};
//    primitives[3].center = {0.1,0.2,0.0f,0};
//    primitives[3].radius = .3f;
//    primitives[3].refractive = {1.1,0,0,1};
    
    return 1;
}

void Pathtracer::set_bvh() {
    bvh = BoundingVolumeHierarchy(vector_triangles);
    bvh.buildTree();
    return;
}