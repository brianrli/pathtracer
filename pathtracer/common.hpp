// [ Common Data Structures ]
#ifndef pathtracer_common_h
#define pathtracer_common_h

#include <Opencl/cl.hpp>
#include "vecmath/mat.h"
#include "vecmath/vec.h"
#include <boost/compute.hpp>

typedef unsigned char Color;

struct Pixel {
    float red;
    float green;
    float blue;
};

struct Bitmap {
    Pixel *pixels;
    int width;
    int height;
};

// cl struct prototypes
struct Primitive {
    cl_float4 refractive;
    cl_float4 specular;
    cl_float4 diffuse;
    cl_float4 emissive;

    cl_float4 center;
    cl_float3 type;
    cl_float radius;
    cl_float4 plane_normal;
};

struct Triangle {
    cl_float3 v1;
    cl_float3 v2;
    cl_float3 v3;
    cl_float3 material;
};

struct Material {
    cl_float4 refractive;
    cl_float4 specular;
    cl_float4 diffuse;
    cl_float4 emissive;
};

struct Camera {
    cl_float4 eye;
    cl_float4 n;
    cl_float4 u;
    cl_float4 v;
};

#endif
