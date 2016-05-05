// [ Common Data Structures ]
#ifndef pathtracer_common_h
#define pathtracer_common_h

#include <Opencl/cl.hpp>
#include "vecmath/mat.h"
#include "vecmath/vec.h"
//#include <boost/compute.hpp>

typedef unsigned char Color;

// [ Image Construction ]
struct Pixel {
    float red;
    float green;
    float blue;
};

struct WPixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Bitmap {
    Pixel *pixels;
    int width;
    int height;
};

// [ Geometry ]
struct BoundingBox {
    BoundingBox() {}
    BoundingBox(Vec3f v) {
        minbounds = v;
        maxbounds = v;
    }

    int longestAxis() {
        int axis = 0;
        if ((maxbounds[1]-minbounds[1])>
            (maxbounds[0]-minbounds[0]))
            axis = 1;

        if ((maxbounds[2]-minbounds[2])>
            (maxbounds[axis]-minbounds[axis]))
            axis = 2;

        return axis;
    }

    float surfaceArea() {
        Vec3f v = maxbounds - minbounds;
        return 2.0f * (v[0] * v[1] + v[0]
                       * v[2] + v[1] * v[2]);
    }
    
    void print() {
        std::cout << "minbounds: ";
        minbounds.print();
        std::cout << "maxbounds: ";
        maxbounds.print();
    }

    Vec3f minbounds;
    Vec3f maxbounds;
};

inline BoundingBox combine(BoundingBox b1, BoundingBox b2) {
    BoundingBox bbox;
    bbox.minbounds = minimum(b1.minbounds,b2.minbounds);
    bbox.maxbounds = maximum(b1.maxbounds,b2.maxbounds);
    return bbox;
};

inline BoundingBox combine(BoundingBox b1, Vec3f v1) {
    BoundingBox bbox;
    bbox.minbounds = minimum(b1.minbounds,v1);
    bbox.maxbounds = maximum(b1.maxbounds,v1);
    return bbox;
};

class VectorTriangle {
public:
    Vec3f v1;
    Vec3f v2;
    Vec3f v3;
    Vec3f centroid;
    BoundingBox bbox;
    
    VectorTriangle() {
    }
    
    VectorTriangle(Vec3f vec1, Vec3f vec2,
                   Vec3f vec3) {
        v1 = vec1;
        v2 = vec2;
        v3 = vec3;

        bbox.minbounds = minimum(minimum(v1,v2),v3);
        bbox.maxbounds = maximum(maximum(v1,v2),v3);

        centroid[0] = (bbox.minbounds[0] + bbox.maxbounds[0]) * 0.5;
        centroid[1] = (bbox.minbounds[1] + bbox.maxbounds[1]) * 0.5;
        centroid[2] = (bbox.minbounds[2] + bbox.maxbounds[2]) * 0.5;
    }
    
    void print() {
        v1.print();
        v2.print();
        v3.print();
    }
};

// [ OpenCL Structure Prototypes ]
struct BVH_Node {
    cl_float minboundsx, minboundsy, minboundsz;
    cl_float maxboundsx, maxboundsy, maxboundsz;
    
    cl_float offset;
    cl_float secondChildOffset;
    cl_float nPrimitives;
    cl_float axis;
};

struct BVH_Node_BBox {
    cl_float minboundsx, minboundsy, minboundsz;
    cl_float maxboundsx, maxboundsy, maxboundsz;
};

struct BVH_Node_Info {
    cl_int offset;
    cl_int secondChildOffset;
    cl_int nPrimitives;
    cl_int axis;
};

// ===[ Primitive ]===
//typedef struct {

//    Vector refractive;
//    Vector specular;
//    Vector diffuse;
//    Vector emissive;
//    Vector center;
//    float4 type;
//    float4 plane_normal;
//    float radius;
//    float p1,p2,p3;
//    
//} Primitive;

struct Primitive {
    cl_float4 refractive;
    cl_float4 specular;
    cl_float4 diffuse;
    cl_float4 emissive;

    cl_float4 center;
    cl_float4 type;
    cl_float4 plane_normal;
    
    cl_float radius;
    //useless padding
    cl_float p1,p2,p3;
};

struct Triangle {
    cl_float4 v1; //vertex 1
    cl_float4 e1; //edge 1
    cl_float4 e2; //edge 2
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
