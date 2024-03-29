// 两个向量加法kernel，grid和block均为一维
__global__ void add(float* x, float * y, float* z, int n)
{
    // 获取该线程的全局索引
    int index = threadIdx.x + blockIdx.x * blockDim.x;
    // 步长(线程总数)
    int stride = blockDim.x * gridDim.x;
    for (int i = index; i < n; i += stride)
    {
        z[i] = x[i] + y[i];
    }
}

#include <iostream>
#include "cuda_runtime.h"  

int main()
{
    int N = 1 << 22; //(2^22)
    int nBytes = N * sizeof(float); // 4*2^20 * 4 = 16 MiB 
    // 申请host内存
    float *x, *y, *z;
    x = (float*)malloc(nBytes);
    y = (float*)malloc(nBytes);
    z = (float*)malloc(nBytes);

    // 初始化数据
    for (int i = 0; i < N; ++i)
    {
        x[i] = 10.0;
        y[i] = 20.0;
    }

    // 申请device内存
    float *d_x, *d_y, *d_z;
    cudaMalloc((void**)&d_x, nBytes);
    cudaMalloc((void**)&d_y, nBytes);
    cudaMalloc((void**)&d_z, nBytes);

    // 将host数据拷贝到device
    // cudaMemcpy((void*)d_x, (void*)x, nBytes, cudaMemcpyHostToDevice);
    // cudaMemcpy((void*)d_y, (void*)y, nBytes, cudaMemcpyHostToDevice);
    cudaMemcpyAsync((void*)d_x, (void*)x, nBytes, cudaMemcpyHostToDevice);
    cudaMemcpyAsync((void*)d_y, (void*)y, nBytes, cudaMemcpyHostToDevice);
    // 定义kernel的执行配置
    dim3 blockSize(1024);
    dim3 gridSize(16);
    // 执行kernel
    for(int i = 0; i < 10; i++)
        add << < gridSize, blockSize >> >(d_x, d_y, d_z, N);

    // 将device得到的结果拷贝到host
    // cudaMemcpy((void*)z, (void*)d_z, nBytes, cudaMemcpyDeviceToHost);
    cudaMemcpyAsync((void*)z, (void*)d_z, nBytes, cudaMemcpyDeviceToHost);
    // 检查执行结果
    float maxError = 0.0;
    for (int i = 0; i < N; i++)
        maxError = fmax(maxError, fabs(z[i] - 30.0));
    std::cout << "最大误差: " << maxError << std::endl;

    // 释放device内存
    cudaFree(d_x);
    cudaFree(d_y);
    cudaFree(d_z);
    // 释放host内存
    free(x);
    free(y);
    free(z);

    return 0;
}
