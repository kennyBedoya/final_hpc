#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <iostream>
#include <time.h>
#define SIZE 10


__device__ unsigned int getIdx(dim3* threads, dim3* blocks) {
    int x;
    return threadIdx.x +
           threadIdx.y * (x  = threads->x) +
           threadIdx.z * (x *= threads->y) +
           blockIdx.x  * (x *= threads->z) +
           blockIdx.y  * (x *= blocks->z) +
           blockIdx.z  * (x *= blocks->y);
}

__device__ void gpu_bottomUpMerge(long* source, long* dest, long start, long middle, long end) {
    long i = start;
    long j = middle;
    for (long k = start; k < end; k++) {
        if (i < middle && (j >= end || source[i] < source[j])) {
            dest[k] = source[i];
            i++;
        } else {
            dest[k] = source[j];
            j++;
        }
    }
}

__global__ void gpu_mergesort(long* source, long* dest, long size, long width, long slices, dim3* threads, dim3* blocks) {
    unsigned int idx = getIdx(threads, blocks);
    long start = width*idx*slices, 
         middle, 
         end;

    for (long slice = 0; slice < slices; slice++) {
        if (start >= size)
            break;

        middle = min(start + (width >> 1), size);
        end = min(start + width, size);
        gpu_bottomUpMerge(source, dest, start, middle, end);
        start += width;
    }
}

void mergesortgpu(long* data, int size, dim3 threadsPerBlock, dim3 blocksPerGrid) {
    double total_t;
    long* D_data;
    long* D_swp;
    dim3* D_threads;
    dim3* D_blocks;

   cudaMalloc((void **)&D_data,size*sizeof(long));
   cudaMalloc((void **) &D_swp, size * sizeof(long));
        
   cudaMemcpy(D_data, data, size * sizeof(long), cudaMemcpyHostToDevice);
   
   cudaMalloc((void**) &D_threads, sizeof(dim3));
   cudaMalloc((void**) &D_blocks, sizeof(dim3));

    cudaMemcpy(D_threads, &threadsPerBlock, sizeof(dim3), cudaMemcpyHostToDevice);
    cudaMemcpy(D_blocks, &blocksPerGrid, sizeof(dim3), cudaMemcpyHostToDevice) ;

    long* A =(long*)malloc( size*sizeof(long));

    long nThreads = threadsPerBlock.x * threadsPerBlock.y * threadsPerBlock.z *
                    blocksPerGrid.x * blocksPerGrid.y * blocksPerGrid.z;
   
    clock_t start,end;
    start= clock();
    for (int width = 2; width < (size << 1); width <<= 1) {
        long slices = size / ((nThreads) * width) + 1;

    gpu_mergesort<<<blocksPerGrid, threadsPerBlock>>>(D_data, D_swp, size, width, slices, D_threads, D_blocks);        
    }
    end=clock();
    //
    // Get the list back from the GPU
    //
    cudaMemcpy(A,D_swp, size * sizeof(long), cudaMemcpyDeviceToHost);
    total_t = ((double)(end-start))/CLOCKS_PER_SEC;

    printf("tiempo en paralelo %f\n", total_t);
    
    // Free the GPU memory
    cudaFree(A);
       
}

/* UITLITY FUNCTIONS */
/* Function to print an array */
void printArray(int A[], int size)
{
    int i;
    for (i=0; i < size; i++)
        printf("%d ", A[i]);
    printf("\n");
}

/* Driver program to test above functions */
int main()
{

	dim3 threadsPerBlock;
    dim3 blocksPerGrid;

    threadsPerBlock.x = 32;
    threadsPerBlock.y = 1;
    threadsPerBlock.z = 1;

    blocksPerGrid.x = 8;
    blocksPerGrid.y = 1;
    blocksPerGrid.z = 1;
   
     int size= SIZE;
  
    long* data;
    data = (long*)malloc( size*sizeof(long) );
    for(int j=0;j<size;j++){
        data[j]=rand() % 100 ;
    }
    
 	mergesortgpu(data, size, threadsPerBlock, blocksPerGrid);    
    //printf("Given array is \n");
    //printArray(arr, arr_size);   
    //printf("\nSorted array is \n");
    //printArray(arr, arr_size);
    return 0;
}
