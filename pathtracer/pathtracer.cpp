#include "pathtracer.hpp"

static float ONE_THIRD_RT = 0.57735026919f;

void printclf4(cl_float4 f) {
    printf("%f %f %f %f   ",f.x,f.y,f.z,f.w);
}

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
    triangle.v1 = (cl_float4){0,0,0,0};
    triangle.e1 = (cl_float4){0,0,0,0};
    triangle.e2 = (cl_float4){0,0,0,0};
    return triangle;
}

Triangle create_triangle(Vec3f v1, Vec3f v2, Vec3f v3)
{
    Triangle triangle;

    Vec3f e1 = v2 - v1;
    Vec3f e2 = v3 - v1;
    
    triangle.v1 = (cl_float4){v1[0],v1[1],v1[2],0.0f};
    triangle.e1 = (cl_float4){e1[0],e1[1],e1[2],0.0f};
    triangle.e2 = (cl_float4){e2[0],e2[1],e2[2],0.0f};

    return triangle;
}

void set(cl_float4 &dst, Vec4f vec) {
    for (int i = 0; i < 4; i++) {
        dst.s[i] = vec[i];
    }
}

// constructor
Pathtracer::Pathtracer(int w, int h) {

    ocl_manager = new OpenCL_Manager();
    ocl_manager->initialize(w,h);
    ocl_manager->kernel_load("pathtracer_kernel.cl",
                             "pathtracer_kernel");

    width = w;
    height = h;
    
    iterations = 0;
    
    seed_memory = new int[width*height*2];
    for(int i = 0; i < (width*height); i++) {
        seed_memory[i] = (rand() % 2147483646)+1;
    }

    image_data = new Pixel[width*height];

    // new
    //first bounce information
    int dim = width * height;

    first_bounce_origin = new cl_float4[dim];
    first_bounce_direction = new cl_float4[dim];
    first_bounce_color = new cl_float4[dim];
    first_bounce_weight = new cl_float4[dim];
    
    // ray information
    origin = new cl_float4[width*height];
    direction = new cl_float4[width*height];
    
    // color information
    color = new cl_float4[width*height];
    weight = new cl_float4[width*height];
    bounce = new int[width*height];
    
    iteration_memory = new int[width*height];
    
    for(int i = 0; i < width*height; i++) {
        iteration_memory[i] = 1;
        
        bounce[i] = 0;
        
        first_bounce_origin[i] = (cl_float4) {0.0,0.0,0.0,0.0};
        first_bounce_direction[i] = (cl_float4) {0.0,0.0,0.0,0.0};
    }
    

    
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



// sets initial values
// wraps initial ray kernel execute
void Pathtracer::iterative_render_first_bounce() {
    ocl_manager->initial_ray_kernel_execute(primitives,camera,n_primitives,
                                            width,height,seed_memory,iterations,
                                            triangles,n_triangles,
                                            materials, n_materials,
                                            bvh_nodes,n_nodes,first_bounce_color,
                                            first_bounce_origin,first_bounce_direction,
                                            first_bounce_weight);
    
//    for (int i = 0; i < width*height; i++) {
////        if(first_bounce_color[i].s3 != 1.0f) {
//            printclf4(first_bounce_color[i]);
////            printclf4(first_bounce_origin[i]);
//            printf("\n");
////        }
//    }
}

void Pathtracer::iterative_render() {
    
//    for (int i = 0; i < width*height; i++) {
//        printclf4(first_bounce_weight[i]);
//        printf(" %d\n",bounce[i]);
//    }
    // [ Path Trace !! ]
    ocl_manager->ray_kernel_execute(primitives, camera, n_primitives, width,
                                    height, seed_memory, iterations, triangles,n_triangles,
                                    materials, n_materials, n_nodes, color, origin, direction,
                                    weight, bounce,first_bounce_color, first_bounce_origin,
                                    first_bounce_direction, first_bounce_weight, iteration_memory);
    iterations++;
    
//    for (int i = 0; i < width*height; i++) {
//        printclf4(color[i]);
//        printf(" %d\n",bounce[i]);
//    }
}

void Pathtracer::reconstruct() {
    ocl_manager->reconstruct_kernel_execute(image_data, width*height, color, iteration_memory);
}

// fake render
Pixel* Pathtracer::render() {
    
    // [ build bounding volume hierarchy ]
    
    // testing
    Pixel *image = new Pixel[width*height];
    
    // [ Path Trace !! ]
    ocl_manager->kernel_execute(primitives,camera,n_primitives,
                                image,width,height,seed_memory,iterations,
                                triangles,n_triangles,materials,n_materials,
                                bvh_nodes,n_nodes); // new bvh additions
    
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
//    Vec4f lookat = Vec4f(24.282, 32.6095, -1.10804, 0);
    Vec4f eye = Vec4f(0,0,-200,0);
//    Vec4f eye = Vec4f(0,0,-0.05f,0);

//    Vec4f lookat = Vec4f(-0.003410, 0.130320, 0.021754,0);
//    Vec4f eye = Vec4f(-0.003410, 0.130320, 0.011754,0);
    
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
    
    if (vertices.size() == 0) {
        std::cout << "no vertices\n";
        return 0;
    }
    
    // has to be divsible by three
    if ((vertices.size() % 3) != 0 ){
        return 0;
    }
    
    // initialize triangles
    for (int i = 0; i < vertices.size(); i += 3) {
        vector_triangles.push_back(VectorTriangle(vertices[i],vertices[i+1],
                                                  vertices[i+2]));
    }
//
    // [ Bounding Volume Hierarchy ]
    bvh_constructor = BoundingVolumeHierarchy(vector_triangles);

    bvh_constructor.buildTree();
    
    bvh_nodes = bvh_constructor.get_bvh();
   
    bvh_bbox = bvh_constructor.get_bvh_bbox();
    bvh_info = bvh_constructor.get_bvh_info();
    
    n_nodes = bvh_constructor.get_n_nodes();
    
    ocl_manager->bvh_to_texture(bvh_bbox, bvh_info, n_nodes);


    // [ Triangle ]
    std::vector<VectorTriangle> ovt = bvh_constructor.get_triangles();
    n_triangles = vertices.size() / 3;
    
    triangles = (Triangle*)malloc(sizeof(Triangle) * n_triangles);

    for (int i = 0; i < ovt.size(); i++) {
        triangles[i] = create_triangle(ovt[i].v1,ovt[i].v2,ovt[i].v3);
//        ovt[i].print();
    }
    
    // prepare texture
    ocl_manager->triangles_to_texture(triangles, n_triangles);
    
    return 1;
}

// set up (amazing) fake scene
int Pathtracer::set_scene() {
    
    // [ Material ]
    n_materials = 1;
    materials = (Material*)malloc(sizeof(Material) * n_materials);
    materials[0] = create_material();
    materials[0].diffuse = {0.6,0.4,0.2,0.0};
//materials[0].diffuse = {1.0,1.0,1.0,0};
//materials[0].specular = {1.0,1.0,1.0,0};
//materials[0].specular = {0.7,0.7,0.2,0};
//materials[0].refractive = {1.0,0,0,2.9};
    
    // [ Primitives ]
    n_primitives = 5;
    primitives = (Primitive *) malloc(sizeof(Primitive)*n_primitives);
    
//    // ground plane
    primitives[0] = create_primitive();
    primitives[0].type = {4,0,0,0};
    primitives[0].center = {0,8,0,0};
    primitives[0].plane_normal = {0,-1,0,0};
    primitives[0].diffuse = {0.6,0.6,0.6,0};
    
    primitives[4] = create_primitive();
    primitives[4].type = {4,0,0,0};
    primitives[4].center = {0,0,1.5,0};
    primitives[4].plane_normal = {0,0,1,0};
    primitives[4].diffuse = {0.8,0.8,0.95,0};
//
    // create light
    primitives[1] = create_primitive();
    primitives[1].center = (cl_float4){-0.5,0.0,0,0};
    primitives[1].type = {0,0,0,0};
    primitives[1].radius = .4;
    primitives[1].emissive = {10,10,10,10};
    
//    // reflective sphere
    primitives[2] = create_primitive();
    primitives[2].type = {0,0,0,0};
    primitives[2].center = (cl_float4){-0.5,0.6,0,0};
    primitives[2].radius = .4;
    primitives[2].diffuse = {0.7,0.7,0.2,0};
    primitives[2].specular = {0.7,0.7,0.2,0};
    primitives[2].refractive = {1.9,0,0,0.0};
//
    primitives[3] = create_primitive();
    primitives[3].type = {0,0,0,0};
    primitives[3].center = {0.5,0.6,0.0f,0};
    primitives[3].radius = .4f;
    primitives[3].diffuse = {0.4,0.4,0.4,0};
    
//
//    // refractive sphere
//    primitives[3] = create_primitive();
//    primitives[3].type = {0,0,0};
//    primitives[3].center = {0.1,0.2,0.0f,0};
//    primitives[3].radius = .3f;
//    primitives[3].refractive = {1.1,0,0,1};
    
    return 1;
}