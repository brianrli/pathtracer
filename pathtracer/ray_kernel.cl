// all in world space for now
#define DEBUG true
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#define EPSILON 0.001f

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef float4 Vector;

// Constants
__constant int MAXDEPTH = 15;
__constant float ONE_THIRD_RT = 0.57735026919f;
__constant Vector BACKGROUND_COLOR = {0.8f,0.9f,0.95f,0.0f};


// ===[ Debugging ]===
void print_vec(Vector vec);
void print_vec2(Vector vec, Vector vec1);
void print_vec(Vector vec) {
    printf("x: %f y: %f z: %f w: %f\n",
           vec.x,vec.y,vec.z,vec.w);
}
void print_vec2(Vector vec, Vector vec1) {
    printf("x: %f y: %f z: %f w: %f | x: %f y: %f z: %f w: %f\n",
           vec.x,vec.y,vec.z,vec.w, vec1.x,vec1.y,vec1.z,vec1.w);
}
void print_float3(float3 vec) {
    printf("x: %f y: %f z: %f ",vec.x,vec.y,vec.z);
}
void print_float4(float4 vec) {
    printf("x: %f y: %f z: %f w: %f\n",vec.x,vec.y,vec.z,vec.w);
}

// ===[ Pixel ]===
typedef struct {
    float red;
    float green;
    float blue;
} Pixel;

void setPixel(Pixel *p, Vector color);
void setPixel(Pixel *p, Vector color) {
    p->red = color.x;
    p->green = color.y;
    p->blue = color.z;
}

// ===[ Camera ]===
typedef struct {
    Vector eye;
    Vector n;
    Vector u;
    Vector v;
} Camera;

// ===[ Ray ]===
typedef enum raytype {PRIMARY, SHADOW, REFLECTION, REFRACTIVE} Raytype;
typedef struct {
    Vector origin;
    Vector direction;
    Raytype type;
    int depth;
    float t;
} Ray;

void print_ray(Ray r);
void print_ray(Ray r) {
    printf("\n");
    print_vec(r.origin);
    print_vec(r.direction);
    printf("t: %f\n",r.t);
}

Ray create_ray(int d, Raytype rt);
Ray create_ray(int d, Raytype rt) {
    Ray ray;
    ray.type = rt;
    ray.depth = d;
    return ray;
}

Vector point_at(Ray r, float t);
Vector point_at(Ray r, float t) {
    return r.origin + (r.direction * t);
};

// ===[ Bounding Volume Hierarchy ]===

typedef struct {
    float minboundsx, minboundsy, minboundsz;
    float maxboundsx, maxboundsy, maxboundsz;
    
    int offset;
    int secondChildOffset;
    int nPrimitives;
    int axis;
} BVH_Node;

typedef struct {
    float minboundsx, minboundsy, minboundsz;
    float maxboundsx, maxboundsy, maxboundsz;
} BVH_Node_BBox;

typedef struct {
    int offset;
    int secondChildOffset;
    int nPrimitives;
    int axis;
} BVH_Node_Info;

void print_node(BVH_Node b) {
    printf("x: %f y: %f z: %f ",b.minboundsx,b.minboundsy,b.minboundsz);
    printf("x: %f y: %f z: %f ",b.maxboundsx,b.maxboundsy,b.maxboundsz);
    printf("off %d   secondchild %d   nprim %d    axis %d\n",b.offset,
           b.secondChildOffset,b.nPrimitives, b.axis);
}


void print_bvh(BVH_Node_BBox box, BVH_Node_Info info) {
    printf("x: %f y: %f z: %f ",box.minboundsx,box.minboundsy,box.minboundsz);
    printf("x: %f y: %f z: %f ",box.maxboundsx,box.maxboundsy,box.maxboundsz);
    printf("off %d   secondchild %d   nprim %d    axis %d\n",info.offset,
           info.secondChildOffset,info.nPrimitives, info.axis);
}


BVH_Node get_node(__read_only image1d_t texture_bvh_bbox,
                  __read_only image1d_t texture_bvh_info,
                  int index);
BVH_Node get_node(__read_only image1d_t texture_bvh_bbox,
                  __read_only image1d_t texture_bvh_info,
                  int index) {
    
    int i = index * 2;
    
    BVH_Node node;
    float4 minbounds = read_imagef(texture_bvh_bbox,i);
    node.minboundsx = minbounds.x;
    node.minboundsy = minbounds.y;
    node.minboundsz = minbounds.z;
    
    float4 maxbounds = read_imagef(texture_bvh_bbox,i+1);
    node.maxboundsx = maxbounds.x;
    node.maxboundsy = maxbounds.y;
    node.maxboundsz = maxbounds.z;
    
    int4 info = read_imagei(texture_bvh_info,index);
    node.offset = info.x;
    node.secondChildOffset = info.y;
    node.nPrimitives = info.z;
    node.axis = info.w;
    
    return node;
}

// ===[ Triangle ]===
typedef struct {
    float4 v1;
    float4 e1;
    float4 e2;
} Triangle;


Triangle get_triangle(__read_only image1d_t texture_triangles, int index) {
    index *= 3;
    Triangle tri;
    tri.v1 = read_imagef(texture_triangles,sampler,index);
    tri.e1 = read_imagef(texture_triangles,sampler,index+1);
    tri.e2 = read_imagef(texture_triangles,sampler,index+2);
    return tri;
    
}

void print_triangle(Triangle tri);
void print_triangle(Triangle tri) {
    print_float4(tri.v1);
    print_float4(tri.e1);
    print_float4(tri.e2);
    //    printf("\n");
}

void print_triangle2(Triangle tri, Triangle tri2);
void print_triangle2(Triangle tri, Triangle tri2) {
    print_triangle(tri);
    print_triangle(tri2);
    printf("\n");
}

typedef struct {
    Vector refractive;
    Vector specular;
    Vector diffuse;
    Vector emissive;
} Material;

// ===[ Primitive ]===
typedef struct {
    // [ refractive / transmissive ]
    // w : 1 if transmissive
    // x : index of refraction
    Vector refractive;
    
    // [ specular ]
    // w : shininess
    Vector specular;
    
    // [ diffuse ]
    Vector diffuse;
    
    // [ light ]
    // x : intensity
    // w : 1 if light
    Vector emissive;
    
    // [ geometry ]
    Vector center;
    
    float4 type;
    // [ Sphere  0 ]
    
    // [ Plane  1 ]
    float4 plane_normal;
    
    float radius;
    float p1,p2,p3;
    
} Primitive;


Material material_from_primitive(Primitive p);
Material material_from_primitive(Primitive p) {
    Material m;
    m.refractive = p.refractive;
    m.specular = p.specular;
    m.diffuse = p.diffuse;
    m.emissive = p.emissive;
    return m;
}

bool islight(Material m);
bool isrefractive(Material m);
bool isspecular(Material m);

bool islight(Material m){
    return (m.emissive.s1 > 0 ||
            m.emissive.s2 > 0 ||
            m.emissive.s3 > 0) ? true : false;
}

bool isspecular(Material m){
    return (any(m.specular > EPSILON)) ? true : false;
}

bool isrefractive(Material m){
    return (m.refractive.s3 == 1) ? true : false;
}

// ===[ Monte Carlo ]===
float rand(int* seed);
float rand(int* seed) // 1 <= *seed < m
{
    int const a = 16807; //ie 7**5
    int const m = 2147483647; //ie 2**31-1
    *seed = (unsigned long)(*seed * a) % m;
    return (float)(*seed)/(float)m;
}

Vector randomSampleHemisphere(Vector normal, int *seed);
Vector randomSampleHemisphere(Vector normal, int *seed) {
    
    // uniformly sample unit sphere
    float z = rand(seed);
    float r2 = rand(seed);
    
    // pythagorean
    float r = sqrt(z);
    float phi = 2 * M_PI_F * r2;
    
    float x = cos(phi) * r;
    float y = sin(phi) * r;
    z = sqrt(1 - z);
    
    //x y z coordinates
    Vector most_perpendicular;
    
    if(fabs(normal.x) < ONE_THIRD_RT) {
        most_perpendicular = (float4) {1,0,0,0};
    }
    else if(fabs(normal.y) < ONE_THIRD_RT) {
        most_perpendicular = (float4) {0,1,0,0};
    }
    else {
        most_perpendicular = (float4) {0,0,1,0};
    }
    
    Vector u = normalize(cross(most_perpendicular,normal));
    Vector v = cross(normal, u);
    
    //    print_vec2(normal, u*x + v*y + normal*z);
    
    return (u*x + v*y + normal*z);
}


// ===[ Intersect Information ]===
typedef struct {
    Vector normal;
    Vector point;
    local Primitive *primitive;
    Triangle triangle;
    bool is_triangle;
} Isect;

// ===[ Intersect Triangles (Moller Trumbore) ]===
// forward declaration
bool intersect_tri(Ray *r, Triangle tri, float *t, Isect *isect);

// WITH ACCELERATION
bool intersectBox(Ray *r, float3 minbounds, float3 maxbounds, Vector invDir,
                  bool dirIsNeg[3], float t);
bool intersectBox(Ray *r, float3 minbounds, float3 maxbounds, Vector invDir,
                  bool dirIsNeg[3], float t) {
    
    // Check for ray intersection against x and y slabs
    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    
    // x slab
    if (dirIsNeg[0]) {
        tmin = (maxbounds.x - (*r).origin.x) * invDir.x;
        tmax = (minbounds.x - (*r).origin.x) * invDir.x;
    }
    else {
        tmin = (minbounds.x - (*r).origin.x) * invDir.x;
        tmax = (maxbounds.x - (*r).origin.x) * invDir.x;
    }
    
    // y slab
    if (dirIsNeg[1]) {
        tymin = (maxbounds.y - (*r).origin.y) * invDir.y;
        tymax = (minbounds.y - (*r).origin.y) * invDir.y;
    }
    else {
        tymin = (minbounds.y - (*r).origin.y) * invDir.y;
        tymax = (maxbounds.y - (*r).origin.y) * invDir.y;
    }
    
    
    
    if ((tmin > tymax) || (tymin > tmax))
        return false;
    
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    
    // Check for ray intersection against z slab
    if (dirIsNeg[2]) {
        tzmin = (maxbounds.z - (*r).origin.z) * invDir.z;
        tzmax = (minbounds.z - (*r).origin.z) * invDir.z;
    }
    else {
        tzmin = (minbounds.z - (*r).origin.z) * invDir.z;
        tzmax = (maxbounds.z - (*r).origin.z) * invDir.z;
    }
    
    
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    
    if (tzmin > tmin)
        tmin = tzmin;
    
    if (tzmax < tmax)
        tmax = tzmax;
    
    // no mint ~~
    return (tmin < t) && (tmax > EPSILON);
}

bool intersect_tri_bvh(Ray *r, __read_only image1d_t triangles, int n_triangles, float *t, Isect *isect,
                       __read_only image1d_t bvh_texture_bbox,
                       __read_only image1d_t bvh_texture_info, int n_nodes);
bool intersect_tri_bvh(Ray *r, __read_only image1d_t triangles, int n_triangles, float *t, Isect *isect,
                       __read_only image1d_t bvh_texture_bbox,
                       __read_only image1d_t bvh_texture_info, int n_nodes) {
    
    
    int current = 0; // position in stack
    int nodeNum = 0;
    int todo[64];
    
    bool hit = false;
    
    // for special bounding box intersection algorithm
    Vector invDir = (float4){
        1.f/(*r).direction.x,
        1.f/(*r).direction.y,
        1.f/(*r).direction.z,
        0.0f};
    
    bool dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};
    
    // intersect loop
    while (true) {
        
        //        printf("inside %d\n",nodeNum);
        // if inside bounding box
        BVH_Node n = get_node(bvh_texture_bbox,
                              bvh_texture_info,
                              nodeNum);
        
        
        float3 minbounds = {n.minboundsx,
            n.minboundsy,
            n.minboundsz};
        float3 maxbounds = {n.maxboundsx,
            n.maxboundsy,
            n.maxboundsz};
        //
        if (intersectBox(r, minbounds, maxbounds, invDir,dirIsNeg,*t)) {
            
            // if leaf node
            if(n.nPrimitives > 0) {
                
                for (int i = 0; i < n.nPrimitives; i++) {
                    
                    Triangle tri = get_triangle(triangles, n.offset+i);
                    if(intersect_tri(r,tri,t,isect)) {
                        hit = true;
                    }
                }
                
                if (current == 0) {
                    //                    printf("C1\n");
                    break;
                }
                
                nodeNum = todo[--current];
            }
            // if interior node
            else {
                // process children by how close they are
                if(dirIsNeg[(int)n.axis]) {
                    todo[current++] = nodeNum+1;
                    nodeNum = n.secondChildOffset;
                }
                //
                else {
                    todo[current++] = n.secondChildOffset;
                    nodeNum++;
                }
            }
        }
        // missed bounding box
        else {
            if (current == 0) {
                break;
            }
            nodeNum = todo[--current];
        }
    }
    
    
    return hit;
}

// WITHOUT ACCELERATION
bool intersect_tri(Ray *r, Triangle tri, float *t, Isect *isect) {
    
    float4 edge1 = tri.e1;
    float4 edge2 = tri.e2;
    float4 pvec = cross((*r).direction,edge2);
    
    float det = dot(edge1,pvec);
    
    // ray and triangle are parallel
    if(fabs(det) < EPSILON) {
        //        printf("1\n");
        return false;
    }
    
    float invDet = 1/det;
    
    float4 tvec = (*r).origin - tri.v1;
    float u = dot(tvec,pvec) * invDet;
    
    if (u < 0 || u > 1) {
        return false;
    }
    
    float4 qvec = cross(tvec,edge1);
    float v = dot((*r).direction.xyz,qvec.xyz) * invDet;
    
    if (v < 0 || u+v > 1) {
        //        printf("3\n");
        return false;
    }
    
    float tt = dot(edge2,qvec) * invDet;
    
    if (tt < *t && tt > 0.0f) {
        
        *t = tt;
        
        (*isect).point = point_at(*r,*t);
        
        float4 n = normalize(cross(edge1,edge2));
        
        // if facing in same direction, flip
        if(dot(n,(*r).direction) > 0.0f) {
            n = -n;
        }
        
        (*isect).normal = n;
        (*isect).triangle = tri;
        (*isect).is_triangle = true;
        return true;
    }
    
    return false;
}

// intersect Primitives
bool intersect(Ray *r,local Primitive *p, float *t, Isect *isect);
bool intersect(Ray *r,local Primitive *p, float *t, Isect *isect) {
    
    float discriminant;
    
    // [ intersect sphere ]
    if ((*p).type.x == 0) {
        
        // d . d
        float a = dot((*r).direction,(*r).direction);
        
        // 2d . (e - c)
        float b = 2 * dot((*r).direction,(*r).origin-(*p).center);
        
        // (e - c) . (e - c) - (R^2)
        float c = dot((*r).origin-(*p).center,(*r).origin-(*p).center)-
        ((*p).radius * (*p).radius);
        
        discriminant = (b*b)-(4*a*c);
        float tt = 1000.0f;
        
        // no intersection
        if (discriminant < 0.0f) {
            return false;
        }
        // intersection at one point
        else if (discriminant >= 0 && discriminant< EPSILON){
            
            tt = -b / (2 * a);
            
        }
        // intersection at two points
        else {
            discriminant = sqrt(discriminant);
            
            float t0 = (-b - discriminant) / (2*a);
            
            if (t0 > EPSILON) {
                tt = t0;
            }
            else {
                tt = (-b + discriminant) / (2*a);
            }
            
        }
        
        // intersect found
        if (tt < *t && tt > 0.0f) {
            *t = tt;
            (*isect).point = point_at(*r,*t);
            (*isect).normal = normalize((*isect).point - (*p).center);
            (*isect).primitive = p;
            return true;
        }
    }
    // Plane
    else if ((*p).type.x == 1) {
        
        float tt = 1000;
        float denom = dot(p->plane_normal, (*r).direction);
        
        if (fabs(denom) > EPSILON) {
            
            Vector between = p->center - (*r).origin;
            tt = dot(between, p->plane_normal) / denom;
            
            // intersect found
            if (tt < *t && tt >= 0) {
                *t = tt;
                (*isect).point = point_at(*r,*t);
                (*isect).normal = normalize(p->plane_normal);
                (*isect).primitive = p;
                
                return true;
            }
        }
    }
    
    return false;
}

// [ Diffuse ]
Ray handle_diffuse(Isect isect, Ray r, int *seed);
Ray handle_diffuse(Isect isect, Ray r, int *seed) {
    Ray new_ray = create_ray(r.depth+1,PRIMARY);
    new_ray.direction = randomSampleHemisphere(isect.normal,seed);
    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
    return new_ray;
}

// [ Reflective ]
Ray handle_reflective(Isect isect, Ray r);
Ray handle_reflective(Isect isect, Ray r) {
    
    Ray new_ray = create_ray(r.depth+1,PRIMARY);
    new_ray.direction = normalize(r.direction -
                                  (2 * dot(isect.normal,r.direction) * isect.normal));
    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
    return new_ray;
}

// [ Refractive ]
Vector refract(Vector D, Vector N, float index);
Vector refract(Vector D, Vector N, float index){
    
    Vector t = (float4) {0.0f,0.0f,0.0f,0.0f};
    
    //index = n/n_t
    float a = 1-(pow(index,2)*(1-pow(dot(D,N),2)));
    
    if(a < 0.0f){
        return t;
    }
    
    t = index * (D-(N*dot(D,N))) - N*sqrt(a);
    t = normalize(t);
    t.w = 1.f;
    
    return t;
}

//get refractive index
float reindex(Material m);
float reindex(Material m) {
    return m.refractive.x;
}

float schlick_fresnel(Isect isect, Ray r, Material m);
float schlick_fresnel(Isect isect, Ray r, Material m) {
    
    float n2 = reindex(m);
    float F0 = pow((1-n2)/(1+n2),2);
    float result;
    
    // entering material
    if(dot(r.direction,isect.normal) < 0.0f){
        result = F0 + (1-F0) * pow(1-dot(isect.normal,-r.direction),5);
    }
    // leaving material
    else {
        result = F0 + (1-F0) * pow(1-dot(isect.normal,r.direction),5);
    }
    
    
    return result;
}

Ray handle_refractive(Isect isect, Ray r, Material m);
Ray handle_refractive(Isect isect, Ray r, Material m) {
    
    Ray new_ray = create_ray(r.depth+1,REFRACTIVE);
    
    Vector N = isect.normal;
    Vector d = r.direction;
    
    float n = reindex(m);
    
    //new vector declarations
    Vector t = (float4) {0.0f,0.0f,0.0f,0.0f};
    
    // entering material
    if(dot(d,N) < 0.0f){
        t = refract(d,N,1/n);
    }
    // leaving material
    else{
        t = refract(d,-N,n);
    }
    
    // refracted
    if(t.w == 1.0f) {
        t.w = 0;
        new_ray.direction = t;
    }
    // total internal reflection
    // (just reflection equation)
    else {
        float ndoti = dot(N,-d);
        new_ray.direction = (ndoti + ndoti)*N + d;
    }
    
    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
    return new_ray;
}

// render loop
//Vector pathtrace(Ray ray,global Primitive *primitives, int n_primitives) {
float4 pathtrace(Ray *ray,local Primitive *primitives, int n_primitives,
                 __read_only image1d_t texture_triangles, int n_triangles, local Material *materials,
                 int n_materials,  __read_only image1d_t texture_bvh_bbox,
                 __read_only image1d_t texture_bvh_info,int n_nodes, int *seed, int *bounce,
                 float4 c, float4 *w);
float4 pathtrace(Ray *ray,local Primitive *primitives, int n_primitives,
                 __read_only image1d_t texture_triangles, int n_triangles, local Material *materials,
                 int n_materials,  __read_only image1d_t texture_bvh_bbox,
                 __read_only image1d_t texture_bvh_info,int n_nodes, int *seed, int *bounce,
                 float4 c, float4 *w) {
    
    // intersection
    float4 color = c;
    float4 weight = *w;
    Ray r = *ray;
    
    float t = 1000;
    Isect isect;
    isect.is_triangle = false;
    bool found_intersect = false;
    Ray new_ray;
    
    // trace ray to find point of intersection
    // with the nearest surface
    for(int i = 0; i < n_primitives; i++) {
        // if there is an intersection
        if(intersect(&r,&(primitives[i]),&t,&isect)) {
            found_intersect = true;
        }
    }
    
    // bounding volume hierarchy
    if(intersect_tri_bvh(&r,texture_triangles,n_triangles,&t,&isect,texture_bvh_bbox,
                         texture_bvh_info,n_nodes)) {
        found_intersect = true;
    }
    
    // [ Process Intersection ]
    if (found_intersect) {
        
        Material m;
        
        if(isect.is_triangle) {
            m = materials[0];
        }
        else {
            m = material_from_primitive((*isect.primitive));
        }
        
        // =====[ Emission (light) ]=====
        if(islight(m)) {
            color += weight * m.emissive;
            color.w = 1.0f;
        }
        else {
            // =====[ Shading ]=====
            
            bool fresnel = rand(seed) < schlick_fresnel(isect,r,m);
            // [ Reflection]
            if(fresnel && isspecular(m)) {
                new_ray = handle_reflective(isect,r);
            }
            // [ Transmission / TIR ]
            else if(isrefractive(m)) {
                if(fresnel) {
                    new_ray = handle_reflective(isect,r);
                }
                else {
                    new_ray = handle_refractive(isect,r,m);
                }
            }
            // [ Diffuse ]
            else {
                weight *= m.diffuse;
                new_ray = handle_diffuse(isect,r,seed);
                float dum = dot(isect.normal,new_ray.direction);
                weight *= (float4){dum,dum,dum,0.0f};
                
            }
        }
    }
    else {
        color+= weight * BACKGROUND_COLOR;
        color.w = 1.0f;
    }
    
    // [ Russian Roulette ]
    if (*bounce > 3 && color.w != 1.0f) {
        float m = (weight.x > weight.y) ? weight.x : weight.y;
        m = (weight.z > m) ? weight.z : m;
        float pterm = rand(seed);
        
        if(m < pterm) {
            color.w = 1.0f;
        }
        weight /= m;
    }
    
    (*bounce)++;
    (*ray) = new_ray;
    (*w) = weight;
    
    return color;
}

// main pathtracing kernel
// need to return point
__kernel void ray_kernel(__read_only image1d_t texture_triangles,
                         __read_only image1d_t texture_bvh_bbox,
                         __read_only image1d_t texture_bvh_info,
                          
                         const __global float4* global_primitives,
                         const __global float4* global_materials,
                          
                         __global int* seed_memory,
                         __global int* iteration_memory,
                         __global Camera* camera,
                          
                         // iterative approach
                         __global float4 *first_color,
                         __global float4 *first_origin,
                         __global float4 *first_direction,
                         __global float4 *first_weight,
                         
                         __global float4 *color,
                         __global float4 *origin,
                         __global float4 *direction,
                         __global float4 *weight,
                         __global int *bounce,
                          
                         __local float4* local_primitives,
                         __local float4* local_materials,
                          
                         int width,
                         int height,
                         int iteration,
                         int n_triangles,
                         int n_materials,
                         int n_primitives,
                         int n_nodes) {
    //
    event_t events[2];
    events[0] = async_work_group_copy(local_primitives,
                                      global_primitives,
                                      (size_t) ((sizeof(Primitive) * n_primitives)/sizeof(float4)), 0);
    events[1] = async_work_group_copy(local_materials,
                                      global_materials,
                                      (size_t) ((sizeof(Material) * n_materials)/sizeof(float4)), 0);
    
    wait_group_events(2, events);
    local Primitive *primitives = (local Primitive*) local_primitives;
    local Material *materials = (local Material*) local_materials;
    
    // global id
    int gid = get_global_id(0);
    
    // padding kernel to align item to group
    if (gid >= width * height) {
        return;
    }
    
    // [ Current Bounce Information ]
    int seed = seed_memory[gid];

    int b = bounce[gid];
    float4 w;// = weight[gid];
    float4 c;// = color[gid];

    
    Ray ray = create_ray(PRIMARY,0);
    
    // [ Update State of Path ]
    if (b == 0) {
        ray.origin = first_origin[gid];
        ray.direction = first_direction[gid];
        w = first_weight[gid];
        c = first_color[gid];
    }
    else {
        ray.origin = origin[gid];
        ray.direction = direction[gid];
        w = weight[gid];
        c = color[gid];
    }

    // path trace
    float4  final_color;
    
    // still bouncing
    if (c.w != 1.0f) {
        final_color = pathtrace(&ray,primitives,n_primitives,
                                    texture_triangles,n_triangles,
                                    materials,n_materials,
                                    texture_bvh_bbox,
                                    texture_bvh_info,n_nodes,
                                    &seed,&b,c,&w);
    }
    else {
        final_color = c;
//        printf("we out here\n");
    }
    
    // random number generator
    seed_memory[gid] = seed;

    //
    color[gid] = final_color; //color
    
    // terminate
    if (final_color.w == 1.0f) {
        iteration_memory[gid]++;
    }
    
    bounce[gid] = b; //next bounce number
    weight[gid] = w; //weight to start with next time

    //new information for ray
    origin[gid] = ray.origin;
    direction[gid] = ray.direction;
}