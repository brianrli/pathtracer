// all in world space for now
#define DEBUG true
#define EPSILON 0.001f
#define MAXDEPTH 3;

typedef float Matrix[16];
typedef float4 Vector;

__constant Vector BACKGROUND_COLOR = {0.95f,0.95f,0.95f,1.0f};

void print_vec(Vector vec) {
    printf("x: %f y: %f z: %f w: %f\n",
           vec.x,vec.y,vec.z,vec.w);
}

// Pixel
typedef struct {
    uchar red;
    uchar green;
    uchar blue;
    uchar alpha;
} Pixel;

void setPixel(Pixel *p, Vector color) {

    p->red = (uchar)(color.x * 255.0f);
    p->green = (uchar)(color.y * 255.0f);
    p->blue = (uchar)(color.z * 255.0f);
    
    if(DEBUG) {
//        printf("%u %u %u\n",p->red,p->green,p->blue);
    }
}

// Camera
typedef struct {
    Vector eye;
    Vector n;
    Vector u;
    Vector v;
} Camera;

// Ray
typedef struct {
    Vector origin;
    Vector direction;
    float t;
} Ray;

Vector point_at(Ray r, float t) {
    return r.origin + (r.direction * t);
};

// Primitives
typedef struct {
    // [ material ]
    
    // w : shininess
    Vector specular;
    Vector diffuse;
    // x : intensity
    // w : 1 if light
    Vector emissive;
    
    // [ geometry ]
    Vector center;
    float3 padding;
    float radius;
} Primitive;

// Intersect Information
typedef struct {
    Vector normal;
    Vector point;
    local Primitive *primitive;
//    global Primitive *primitive;
} Isect;

bool islight(Primitive p){
    return (p.emissive.s3 == 1) ? true : false;
}

// intersect
//bool intersect(Ray *r,global Primitive *p, float *t, Isect *isect) {
bool intersect(Ray *r,local Primitive *p, float *t, Isect *isect) {
    
    float discriminant;

    // intersect sphere
    if (true) {

        // d . d
        float a = dot((*r).direction,(*r).direction);
        
        // 2d . (e - c)
        float b = 2 * dot((*r).direction,(*r).origin-(*p).center);
        
        // (e - c) . (e - c) - (R^2)
        float c = dot((*r).origin-(*p).center,(*r).origin-(*p).center)-
        ((*p).radius * (*p).radius);

        discriminant = (b*b)-(4*a*c);
        
        // no intersection
        if (discriminant < 0.0f) {
            return false;
        }
        // intersection at one point
        else if (discriminant >= 0 && discriminant< EPSILON){
            
            float tt = -b / (2 * a);
            
            if (tt < *t) {
                (*isect).point = point_at(*r,*t);
                (*isect).normal = normalize((*isect).point - (*p).center);
                (*isect).primitive = p;
            }

            return true;
        }
        // intersection at two points
        else {
            discriminant = sqrt(discriminant);

            float tt = 1000.0f;
            
            float t0 = (-b - discriminant) / (2*a);
            
            if (t0 > EPSILON) {
                tt = t0;
            }
            else {
                tt = (-b + discriminant) / (2*a);
            }
            
            if (tt < *t) {
                *t = tt;
                (*isect).point = point_at(*r,*t);
                (*isect).normal = normalize((*isect).point - (*p).center);
                (*isect).primitive = p;
            }
            
            return true;
        }
    }
    
    return false;
}

// render loop
//Vector pathtrace(Ray ray,global Primitive *primitives, int n_primitives) {
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives) {

    // intersection
    Vector color = {0,0,0,0};

    Isect isect;
    float t = 1000;
    
    bool found_intersect = false;
    
    // loop through primitives
    for(int i = 0; i < n_primitives; i++) {

        // if the primitive is not a light
        if (!islight(primitives[i])) {
            // if there is an intersection
            if(intersect(&ray,&(primitives[i]),&t,&isect)) {
                found_intersect = true;
            }
        }
    }
    
    // loop through lights (no shadows)
    if (found_intersect) {

        for(int j = 0; j < n_primitives; j++) {

            if(islight(primitives[j])) {
                
                Vector to_light = normalize(primitives[j].center - isect.point);
                
                // Diffuse
                float k = dot(to_light,isect.normal);
                if (k > 0.0f) {
                    color += k * primitives[j].emissive.x *
                    (*isect.primitive).diffuse;
                }
                
                // Blinn-Phong Specular
                Vector view = -ray.direction;
                Vector h = normalize(view + to_light);
                float k1 = dot(isect.normal,h);

                if (k1 > 0.0f) {
                    color += pow(k1,1)
                    * primitives[j].emissive.x *
                    (*isect.primitive).specular;

                }
                
                //
            }
        }
    }
    else {
        color = BACKGROUND_COLOR;
    }

    color = clamp(color,0.0f,1.0f);
//    print_vec(color);

    return color;
}

// main pathtracing kernel
__kernel void pathtracer_kernel(const __global float4* global_primitives,
                                int n_primitives,
                                __global Camera* camera,
                                int width,
                                int height,
                                __global Pixel* image,
                                __local float4* local_primitives) {

    // padding kernel to align item to group
    
    
    // perform copy from global to local memory
    event_t event = async_work_group_copy(local_primitives,
                                          global_primitives,
                                          (size_t) ((sizeof(Primitive) * n_primitives)/sizeof(float4)), 0);
    wait_group_events(1, &event);
    local Primitive *primitives = (local Primitive*) local_primitives;
    
    Pixel pixel;
    Vector color;
    Ray ray;
    
    //
    int gid = get_global_id(0);
    
    // calculate pixel coordinates
    float x = fmod((float)gid,(float)width);
    float y = gid / (float)width;
    
    x = x/(float)width - 0.5f;
    y = y/(float)height - 0.5f;
    
    ray.origin = camera->eye;
    ray.direction = (x * camera->u) + (y * camera->v) + -(camera->n);
    
    // ray trace
    color = pathtrace(ray,primitives,n_primitives);
    setPixel(&pixel,color);
    image[gid] = pixel;
}