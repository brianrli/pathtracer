// store common data structures
#ifndef pathtracer_common_h
#define pathtracer_common_h

#include <Opencl/cl.hpp>
#include "vecmath/mat.h"
#include "vecmath/vec.h"

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

struct Primitive {
    cl_float4 center;
    cl_float origin;
    cl_float3 padding;
};

#endif
