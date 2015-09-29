// store common data structures
#ifndef pathtracer_common_h
#define pathtracer_common_h

#include <Opencl/cl.hpp>
#include "vecmath/mat.h"
#include "vecmath/vec.h"
#include <boost/compute.hpp>

typedef unsigned char Color;

struct Pixel {
    Color red;
    Color green;
    Color blue;
};

struct Bitmap {
    Pixel *pixels;
    int width;
    int height;
};

// cl struct prototypes
struct Primitive {
    cl_float4 center;
    cl_float3 padding;
    cl_float radius;
};

struct Camera {
    cl_float4 eye;
    cl_float4 n;
    cl_float4 u;
    cl_float4 v;
};


#endif
