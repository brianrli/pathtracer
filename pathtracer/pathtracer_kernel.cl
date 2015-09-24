// all in world space for now
#define EPSILON 0.001f

// alignment
typedef float Matrix[16];
typedef float4 Vector;

// Scene

// Ray
typedef struct {
    Vector origin;
    Vector direction;
    float t;
    float3 padding;
} Ray;

// Primitives (sphere only)
typedef struct {
    Vector center;
    float radius;
    float3 padding;
} Primitive;

Primitive createSphere() {
    Primitive sphere;
    sphere.center = (float4)(0,0,0,0);
    sphere.radius = 1;
    return sphere;
}

// Functions

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
    Primitive s = createSphere();
    printf("executed %f\n", s.radius);
}

// kernel
__kernel void pathtracer_kernel(__global float4* matrix,
                     __global float4* vector,
                     __global float* result) {
    
    int i = get_global_id(0);
    result[i] = dot(matrix[i], vector[0]);
    
    // lol
    pathtrace();
}