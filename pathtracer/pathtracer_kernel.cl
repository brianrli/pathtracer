// all in world space for now
#define DEBUG true
#define EPSILON 0.001f

typedef float4 Vector;

// Constants
__constant int MAXDEPTH = 8;
__constant float ONE_THIRD_RT = 0.57735026919f;
__constant Vector BACKGROUND_COLOR = {0.5f,0.5f,0.5f,0.0f};


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
    float transparency;
} Ray;

Ray create_ray(int d, Raytype rt);
Ray create_ray(int d, Raytype rt) {
    Ray ray;
    ray.type = rt;
    ray.depth = d;
    ray.transparency = 1.0f;
    return ray;
}

Vector point_at(Ray r, float t);
Vector point_at(Ray r, float t) {
    return r.origin + (r.direction * t);
};

// ===[ Ray Queue ]===
typedef struct {
    Ray data[10];
    int back;
    int front;
} Queue;

Queue create_queue(Ray r);
void push_back(Queue *q, Ray r);
void pop_front(Queue *q);
Ray front(Queue *q);
bool empty(Queue *q);

Queue create_queue(Ray r) {
    Queue queue;
    queue.data[0] = r;
    queue.front = 0;
    queue.back = 1;
    
    return queue;
}

void push_back(Queue *q, Ray r) {
    q->data[q->back] = r;
    q->back++;
}

void pop_front(Queue *q) {
    if (q->front == q->back)
        return;
    q->front++;
}

//queue shift function?
Ray front(Queue *q) {
    return q->data[q->front];
}

bool empty(Queue *q) {
    return q->front == q->back;
}

// ===[ Primitive ]===
typedef struct {
    // [ refractive ]
    // w : 1 if refractive
    // y : transparency coefficient
    // x : index of refraction
    Vector refractive;
    
    // [ reflective ]
    Vector reflective;
    
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

bool islight(Primitive p);
bool isreflective(Primitive p);
bool isrefractive(Primitive p);
    
bool islight(Primitive p){
    return (p.emissive.s1 > 0 ||
            p.emissive.s2 > 0 ||
            p.emissive.s3 > 0) ? true : false;
}

bool isspecular(Primitive p){
    return (any(p.specular > 0)) ? true : false;
}

bool isrefractive(Primitive p){
    return (p.refractive.s3 == 1) ? true : false;
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
    int id;
//    global Primitive *primitive;
} Isect;


// intersect
//bool intersect(Ray *r,global Primitive *p, float *t, Isect *isect) {
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
        
        if (tt < *t && tt > 0.0f) {
    
            if ((*r).type == PRIMARY
                || (*r).type == REFLECTION
                || (*r).type == REFRACTIVE) {
                
                 *t = tt;
                (*isect).point = point_at(*r,*t);
                (*isect).normal = normalize((*isect).point - (*p).center);
                (*isect).primitive = p;
            }
            
            return true;
        }
        else {
            return false;
        }
    }
    // Plane
    else if ((*p).type.x == 1) {
        
        float tt = 1000;
        float denom = dot(p->plane_normal, (*r).direction);
        
        if (fabs(denom) > EPSILON) {
            
            Vector between = p->center - (*r).origin;
            tt = dot(between, p->plane_normal) / denom;

            if (tt < *t && tt >= 0) {
                if ((*r).type == PRIMARY
                    || (*r).type == REFLECTION
                    || (*r).type == REFRACTIVE) {

                    *t = tt;
                    (*isect).point = point_at(*r,*t);
                    (*isect).normal = normalize(p->plane_normal);
                    (*isect).primitive = p;
                }
                
                return true;
            }
        }
        
        return false;
    }
    
    return false;
}

// [ Diffuse ]
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

float schlick_fresnel(Isect isect, Ray r);
float schlick_fresnel(Isect isect, Ray r) {

    float n2 = (*isect.primitive).refractive.x;
    float r0 = pow((1-n2)/(1+n2),2);
    float result;
    
    // entering material
    if(dot(r.direction,isect.normal) < 0.0f){
        result = r0 + (1-r0) * pow(1-dot(isect.normal,-r.direction),5);
    }
    // leaving material
    else {
        result = r0 + (1-r0) * pow(1-dot(isect.normal,r.direction),5);
    }
    return result;
}

Ray handle_refractive(Isect isect, Ray r, float r1);
Ray handle_refractive(Isect isect, Ray r, float r1) {
    
    Ray new_ray = create_ray(r.depth+1,REFRACTIVE);

    Vector N = isect.normal;
    Vector d = r.direction;

    float n =(*isect.primitive).refractive.x;

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
    if(t.w == 1.0f && r1 > schlick_fresnel(isect,r)){
        t.w = 0;
        new_ray.direction = t;
    }
    // total internal reflection
    else {
        float ndoti = dot(N,-d);
        // just reflection equation
        new_ray.direction = (ndoti + ndoti)*N + d;
    }

    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
    return new_ray;
}

// render loop
//Vector pathtrace(Ray ray,global Primitive *primitives, int n_primitives) {
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives, int *seed);
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives, int *seed) {
    
    //create ray queue
    Queue ray_queue = create_queue(ray);

    // intersection
    Vector color = {0.0f,0.0f,0.0f,0.0f};
    Vector weight = {1.0f,1.0f,1.0f,0.0f};
    
    Isect isect;
    bool return_background = true;
    
    int depth = 0;

    while(!empty(&ray_queue)) {
    
        Vector contribution = {0.0f,0.0f,0.0f,0.0f};
        
        float t = 1000;
        bool found_intersect = false;

        Ray r = front(&ray_queue);
        depth = r.depth + 1;

        if (r.depth > MAXDEPTH) {
            break;
        }
        
        // Step 2. trace ray to find point of intersection
        // with the nearest surface
        for(int i = 0; i < n_primitives; i++) {
            // if there is an intersection
            if(intersect(&r,&(primitives[i]),&t,&isect)) {
                isect.id = i;
                found_intersect = true;
                return_background = false;
            }
        }
        
        //
        if (found_intersect) {
            
            // =====[ Emission ( light ) ]=====
            if(islight(*isect.primitive)) {
                color += weight * (*isect.primitive).emissive;
            }
            else {
                // =====[ Shading ]=====

                
                //Transmission / TIR
                if (isrefractive(*isect.primitive)) {
                    push_back(&ray_queue,handle_refractive(isect,r,rand(seed)));
                }
                //Reflection
                else if (isspecular(*isect.primitive)) {
                    weight *= (*isect.primitive).specular;
                    push_back(&ray_queue,handle_reflective(isect,r));
                }
                // Diffuse
                else {
                    Ray new_ray = handle_diffuse(isect,r,seed);
                    weight *= 2.0f * (*isect.primitive).diffuse * dot(isect.normal,new_ray.direction);
                    push_back(&ray_queue,new_ray);
                }
            }
        }
        else {
            color+= weight * BACKGROUND_COLOR;
            break;
        }
    
        pop_front(&ray_queue);
    }

    return color;
}

// main pathtracing kernel
__kernel void pathtracer_kernel(const __global float4* global_primitives,
                                int n_primitives,
                                __global Camera* camera,
                                int width,
                                int height,
                                __global Pixel* image,
                                __local float4* local_primitives,
                                __global int* seed_memory,
                                int iteration) {

    
    // perform copy from global to local memory
    event_t event = async_work_group_copy(local_primitives,
                                          global_primitives,
                                          (size_t) ((sizeof(Primitive) * n_primitives)/sizeof(float4)), 0);
    wait_group_events(1, &event);
    local Primitive *primitives = (local Primitive*) local_primitives;
    
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
    color = pathtrace(ray,primitives,n_primitives, &seed);
    setPixel(&pixel,color);
    
    seed_memory[gid] = seed;
    image[gid] = pixel;
}