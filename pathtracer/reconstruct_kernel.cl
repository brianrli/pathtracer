/*
 [ Image Reconstruction Kernel ]
*/
typedef struct {
    float red;
    float green;
    float blue;
} Pixel;

__kernel void reconstruct_kernel(__global Pixel *image,
                                 __global float4 *contribution,
                                 __global int* iteration_memory)
{
    // no contribution
    int gid = get_global_id(0);
    
    if (gid >= 1000) {
        return;
    }
    
//    if (contribution[gid].w == 1.0f) {
//        return;
//    }
    
    int iterations = iteration_memory[gid];
    printf("%d\n",iterations);
//
    float i1 = 1/(float)iterations;
    float i2 = (float)(iterations-1)/(float)iterations;

    Pixel p;
    p.red = (i1*contribution[gid].x)+(i2*image[gid].red);
    p.blue = (i1*contribution[gid].y)+(i2*image[gid].blue);
    p.green = (i1*contribution[gid].z)+(i2*image[gid].green);
    
    image[gid] = p;
}