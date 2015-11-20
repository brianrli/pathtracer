// all in world space for now
#define DEBUG true
#define EPSILON 0.001f

typedef float4 Vector;

// Constants
__constant int MAXDEPTH = 5;
__constant int SHADOWDEPTH = 2;
__constant Vector BACKGROUND_COLOR = {0.95f,0.95f,0.95f,1.0f};

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

    if(DEBUG) {
//        printf("%u %u %u\n",p->red,p->green,p->blue);
    }
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
    return (p.emissive.s3 == 1) ? true : false;
}

bool isreflective(Primitive p){
    return (p.reflective.s3 == 1) ? true : false;
}

bool isrefractive(Primitive p){
    return (p.refractive.s3 == 1) ? true : false;
}

// ===[ Monte Carlo ]===
int rand(int* seed);
int rand(int* seed) // 1 <= *seed < m
{
    int const a = 16807; //ie 7**5
    int const m = 2147483647; //ie 2**31-1
    *seed = (unsigned long)(*seed * a) % m;
    return (*seed);
}

Vector randomSampleHemisphere(float u1, float u2, Vector normal);
Vector randomSampleHemisphere(float u1, float u2, Vector normal) {
//    float r = sqrt(u1);
//    float theta = 2 * PI * u2;
//    
//    float x = r * cos(theta);
//    float y = r * cos(theta);
//    
    
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

// [ Reflective ]
Ray handle_reflective(Isect isect, Ray r);
Ray handle_reflective(Isect isect, Ray r) {
    
    Ray new_ray = create_ray(r.depth+1,REFLECTION);
    
    new_ray.direction = normalize(r.direction -
                                  (2 * dot(isect.normal,r.direction) * isect.normal));
    
    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
}

// [ Refractive ]
Vector refract(Vector D, Vector N, float index);
Vector refract(Vector D, Vector N, float index){

    Vector t = (float4) {0.0f,0.0f,0.0f,0.0f};
    
    //index = n/n_t
    float a = 1-(pow(index,2)*(1-pow(dot(D,N),2)));

    if(a<0.0){
        return t;
    }
    
    t = index * (D-(N*dot(D,N))) - N*sqrt(a);
    t = normalize(t);
    t.w = 1.f;
    
    return t;
}

Ray handle_refractive(Isect isect, Ray r);
Ray handle_refractive(Isect isect, Ray r) {
    
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

    if(t.w == 1.0f){
        t.w = 0;
        new_ray.direction = t;
    }
    else {
        float ndoti = dot(N,-d);
        new_ray.direction = (ndoti + ndoti)*N + d;
    }

    new_ray.transparency = (*isect.primitive).refractive.y;
    new_ray.origin = isect.point + (EPSILON * new_ray.direction);
    
    return new_ray;
}

// render loop
//Vector pathtrace(Ray ray,global Primitive *primitives, int n_primitives) {
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives, float random_n);
Vector pathtrace(Ray ray,local Primitive *primitives, int n_primitives, float random_n) {
    
    //create ray queue
    Queue ray_queue = create_queue(ray);

    // intersection
    Vector color = {0.0f,0.0f,0.0f,0.0f};
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
        
        // loop through primitives
        for(int i = 0; i < n_primitives; i++) {
            
            // if the primitive is not a light
            if (!islight(primitives[i])) {

                // if there is an intersection
                if(intersect(&r,&(primitives[i]),&t,&isect)) {
                    
                    isect.id = i;
                    found_intersect = true;
                    return_background = false;
                }
            }
        }
        
        // loop through lights and shadow rays
        if (found_intersect) {
            
            // =====[ Reflective Rays ]=====
            //queue new reflective ray
            if (isreflective(*isect.primitive)) {
                push_back(&ray_queue,handle_reflective(isect,r));
            }
            
            //  =====[ Refractive Rays ]=====
            //queue new refractive ray (Snell)
            if  (isrefractive(*isect.primitive)) {
                push_back(&ray_queue,handle_refractive(isect,r));
            }
            
            // =====[ Refractive Rays ]=====
            for(int j = 0; j < n_primitives; j++) {
                
                if(islight(primitives[j])) {
                
                    bool occluded = false;
                    
                    float shadow_t = length(primitives[j].center - isect.point);
                    
                    Ray shadow_ray = create_ray(0,SHADOW);
                    shadow_ray.origin = isect.point;
                    shadow_ray.direction = normalize(primitives[j].center - isect.point);
                    
                    for(int w = 0; w < n_primitives; w++) {
                        if(w != j && w != isect.id && !islight(primitives[w])) {
                            // fire shadow ray
                            if(intersect(&shadow_ray,&(primitives[w]),&shadow_t,0)) {
                                occluded = true;
                                break;
                            }
                        }
                    }
                    
                    
                    // =====[ Shading ]=====
                    if (!occluded) {
                        
                        Vector to_light = normalize(primitives[j].center - isect.point);
                        
                        // hard coded
//                        if((*isect.primitive).type.x == 1.0) {
//                            contribution += 0.5 * (*isect.primitive).diffuse;
//                        }

                        
                        // Diffuse
                        float k = dot(to_light,isect.normal);
                        if (k > 0.0f) {
                            contribution += k * primitives[j].emissive.x *
                            (*isect.primitive).diffuse;
                        }
                        
                        // Blinn-Phong Specular
                        Vector view = -r.direction;
                        Vector h = normalize(view + to_light);
                        float k1 = dot(isect.normal,h);

                        if (k1 > 0.0f) {
                            contribution += pow(k1,(*isect.primitive).specular.w)
                            * primitives[j].emissive.x *
                            (*isect.primitive).specular;

                        }
                    }

                    //
                }
            }
            
        }

        if(r.type == REFRACTIVE) {
            color += r.transparency*contribution;

            if(!found_intersect)
                color += BACKGROUND_COLOR;
        }
        else {
            color += contribution;
        }

        pop_front(&ray_queue);
    }

    if (return_background) color = BACKGROUND_COLOR;
    
//    print_vec(color);
    return clamp(color,0.0f,1.0f);
}

// main pathtracing kernel
__kernel void pathtracer_kernel(const __global float4* global_primitives,
                                int n_primitives,
                                __global Camera* camera,
                                int width,
                                int height,
                                __global Pixel* image,
                                __local float4* local_primitives,
                                __global int* seed_memory) {

    
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
    
    // Monte Carlo == Create a Random Number
    // pnrg
    // http://www0.cs.ucl.ac.uk/staff/ucacbbl/ftp/papers/langdon_2009_CIGPU.pdf
    int seed = seed_memory[gid];

    int new_seed = rand(&seed);
    float rn1 = (float)new_seed/2147483647;
    
    new_seed = rand(&new_seed);
    float rn2 = (float)new_seed/2147483647;

    seed_memory[gid] = new_seed;
    
    // calculate pixel coordinates
    // TODO: add jittering / supersampling
    float x = fmod((float)gid,(float)width);
    float y = gid / (float)width;
    
    x = x/(float)width - 0.5f;
    y = y/(float)height - 0.5f;
    
    ray.origin = camera->eye;
    ray.direction = normalize((x * camera->u) + (y * camera->v) + -(camera->n));
    
    // ray trace
    color = pathtrace(ray,primitives,n_primitives, rn1);
    setPixel(&pixel,color);
 
    image[gid] = pixel;
}