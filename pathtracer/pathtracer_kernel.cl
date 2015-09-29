// all in world space for now
#define EPSILON 0.001f

// alignment
typedef float Matrix[16];
typedef float4 Vector;
typedef uchar Color;

typedef struct {
    Color red;
    Color green;
    Color blue;
} Pixel;

// Camera
typedef struct {
    float4 eye;
    float4 n;
    float4 u;
    float4 v;
} Camera;

// Ray
typedef struct {
    Vector origin;
    Vector direction;
    float t;
    float3 padding;
} Ray;

// Primitives (sphere only)
typedef struct {
    float4 center;
    float radius;
    float3 padding;
} Primitive;

// intersect
int intersect(Ray *r, Primitive *p, float t, Vector *normal) {
    
    int discriminant;

    if (1) {
        float a = dot((*r).direction,(*r).direction);
        float b = dot(2*(*r).direction,(*r).origin-(*p).center);
        float c = dot((*r).origin-(*p).center,(*r).origin-(*p).center);
        discriminant = b*b-4*a*c;
        
        // no intersection
        if (discriminant < -EPSILON) {
            return 0;
        }
        // intersection
        else {
            return 1;
        }
    }
    return 0;
}

// pathtrace
void pathtrace() {
}

// kernel
__kernel void pathtracer_kernel(
    __global Primitive* primitives, 
    const int n_primitives,
    __global Camera* camera,
    __global Pixel* image) {
    
    printf("gpu n primitives: %i\n",n_primitives);

    int i = get_global_id(0);
    
    Pixel pixel;
    pixel.red = 255;
    pixel.green = 10;
    pixel.blue = 10;

    image[i] = pixel;
}