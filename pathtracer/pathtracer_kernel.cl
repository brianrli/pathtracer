__kernel void matvec(__global float4* matrix,
                     __global float4* vector,
                     __global float* result) {
    
    int i = get_global_id(0);
    result[i] = dot(matrix[i], vector[0]);
}

