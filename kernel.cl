__kernel void Teste(const __global float *A, const __global float *B, __global float *C) {
    
    // Thread identifiers
    const int i = get_global_id(0); 
    
    C[i] = A[i] * B[i];
    
}
