// all in world space for now
#define DEBUG true
#define EPSILON 0.001f

typedef float4 Vector;

// Constants
__constant int MAXDEPTH = 15;
__constant float ONE_THIRD_RT = 0.57735026919f;
__constant Vector BACKGROUND_COLOR = {0.5f,0.5f,0.5f,0.0f};


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

// ===[ Triangle ]===
typedef struct {
    float3 v1;
    float3 v2;
    float3 v3;
    float3 material;
} Triangle;

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

    float3 type;
    // [ Sphere  0 ]
    float radius;
    
    // [ Plane  1 ]
    float4 plane_normal;
    
    // [ Box ]
    // todo
    

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

// intersect Triangles (moller trumbore);
bool intersect_tri(Ray *r, local Triangle *tri, float *t, Isect *isect);
bool intersect_tri(Ray *r, local Triangle *tri, float *t, Isect *isect) {
    
    float3 edge1 = tri->v2 - tri->v1; //edge1
    float3 edge2 = tri->v3 - tri->v1; //edge2
    float3 pvec = cross((*r).direction.xyz,edge2);
    
    float det = dot(edge1,pvec);
    
    // ray and triangle are parallel
    if(fabs(det) < EPSILON) {
        return false;
    }
    
    float invDet = 1/det;
    
    float3 tvec = (*r).origin.xyz - tri->v1;
    float u = dot(tvec,pvec) * invDet;

    if (u < 0 || u > 1) {
        return false;
    }
    
    float3 qvec = cross(tvec,edge1);
    float v = dot((*r).direction.xyz,qvec) * invDet;

    if (v < 0 || u+v > 1) {
        return false;
    }
    
    float tt = dot(edge2,qvec) * invDet;
    
    if (tt < *t && tt > 0.0f) {
        
        *t = tt;
        
        (*isect).point = point_at(*r,*t);
        
        float3 n = normalize(cross(edge1,edge2));

        // if facing in same direction, flip
        if(dot(n,(*r).direction.xyz) > 0.0f) {
            n = -n;
        }
        
        (*isect).normal = (float4) {n.x,n.y,n.z,0.0f};
//        print_vec((*isect).normal);
        
        (*isect).triangle = *tri;
//        printf("material %f\n",(isect->triangle->material).x);
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
//        if(isect.is_triangle) {
//            printf("%f\n",1-dot(isect.normal,-r.direction));
////            print_vec2(isect.normal,-r.direction);
////            printf("%f\n",dot(isect.normal,-r.direction));
//            //        printf("%f\n",result);
//        }
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
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives,
                 local Triangle *triangles, int n_triangles, local Material *materials,
                 int n_materials, int *seed);
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives,
                 local Triangle *triangles, int n_triangles, local Material *materials,
                 int n_materials, int *seed) {
    
    // intersection
    Vector color = {0.0f,0.0f,0.0f,0.0f};
    Vector weight = {1.0f,1.0f,1.0f,0.0f};

    Ray r = ray;
    int b;
    for(b = 0; b < MAXDEPTH; b++) {
        
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
        for (int i = 0; i < n_triangles; i++) {
            if(intersect_tri(&r,&(triangles[i]),&t,&isect)) {
                found_intersect = true;
            }
        }
        
        // [ Process Intersection ]
        if (found_intersect) {
            
            Material m;

            if(isect.is_triangle) {
                m = materials[(int)isect.triangle.material.x];
            }
            else {
                m = material_from_primitive((*isect.primitive));
            }

            // =====[ Emission (light) ]=====
            if(islight(m)) {
                color += weight * m.emissive;
                break;
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
                    weight *= dot(isect.normal,new_ray.direction)*2.0;
                    
//                    if(isect.is_triangle) {
//                        printf("%f\n",dot(isect.normal,r.direction));
//                    }
                }
            }
        }
        else {
            color+= weight * BACKGROUND_COLOR;
            break;
        }
        
        // [ Russian Roulette ]
        if (b > 3) {            
            float m = (weight.x > weight.y) ? weight.x : weight.y;
            m = (weight.z > m) ? weight.z : m;
            float pterm = rand(seed);
            
            if(m < pterm) {
                break;
            }
            weight /= m;
        }
        
    
        r = new_ray;
    }
    
    return color;
}

// main pathtracing kernel
__kernel void pathtracer_kernel(const __global float4* global_primitives,
                                const __global float4* global_triangles,
                                const __global float4* global_materials,
                                __global Pixel* image,
                                __global int* seed_memory,
                                __global Camera* camera,
                                __local float4* local_primitives,
                                __local float4* local_materials,
                                __local float4* local_triangles,
                                int width,
                                int height,
                                int iteration,
                                int n_triangles,
                                int n_materials,
                                int n_primitives) {

    
    // perform copy from global to local memory
    event_t events[3];
    events[0] = async_work_group_copy(local_primitives,
                                     global_primitives,
                                     (size_t) ((sizeof(Primitive) * n_primitives)/sizeof(float4)), 0);
    events[1] = async_work_group_copy(local_materials,
                                      global_materials,
                                      (size_t) ((sizeof(Material) * n_materials)/sizeof(float4)), 0);
    events[2] = async_work_group_copy(local_triangles,
                                      global_triangles,
                                      (size_t) ((sizeof(Triangle) * n_triangles)/sizeof(float4)), 0);

    wait_group_events(3, events);
    local Primitive *primitives = (local Primitive*) local_primitives;
    local Material *materials = (local Material*) local_materials;
    local Triangle *triangles = (local Triangle*) local_triangles;
    
    Pixel pixel;
    Vector color;
    Ray ray = create_ray(PRIMARY,0);

    //
    int gid = get_global_id(0);
    
    // padding kernel to align item to group
    if (gid >= width * height) {
        return;
    }
    
    // ======[ Stratified Jittering ]======
    // Monte Carlo == Create a Random Number
    // pnrg
    // http://www0.cs.ucl.ac.uk/staff/ucacbbl/ftp/papers/langdon_2009_CIGPU.pdf
    int seed = seed_memory[gid];

    int n = 6;
    
    // calculate pixel coordinates
    float x = fmod((float)gid,(float)width);
    float y = gid / (float)width;

    x = (x/(float)width) - 0.5f;
    y = (y/(float)height) - 0.5f;

    float left_limit = x - 1/(float)(width + width);
    float top_limit = y - 1/(float)(height + height);
    
    int cell = iteration % (n * n);
    
    float inv_n_w = 1/(float)(n * width);
    float inv_n_h = 1/(float)(n * height);

    x = left_limit + ((cell%n) + rand(&seed)) * inv_n_w;
    y = top_limit + ((cell/n)  + rand(&seed)) * inv_n_h;
    //=======================================
    
    ray.origin = camera->eye;
    ray.direction = normalize((x * camera->u) + (y * camera->v) + -(camera->n));
    
    // ray trace
    color = pathtrace(ray,primitives,n_primitives,
                      triangles,n_triangles,
                      materials,n_materials,&seed);
    setPixel(&pixel,color);
    
    seed_memory[gid] = seed;
    image[gid] = pixel;
}