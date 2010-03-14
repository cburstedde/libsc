
/* originally from http://people.maths.ox.ac.uk/gilesm/ and modified */

//
// include files
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cuda.h>
#include "prac5.h"

//
// kernel routine
//

__global__ void my_first_kernel(float *x)
{
  int tid = threadIdx.x + blockDim.x*blockIdx.x;

  x[tid] = threadIdx.x;
}


//
// CUDA routine to be called by main code
//

int prac5(int nblocks, int nthreads)
{
  float *h_x, *d_x;
  int   nsize, n;

  // allocate memory for arrays

  nsize = nblocks*nthreads ;

  h_x = (float *)malloc(nsize*sizeof(float));
  cudaMalloc((void **)&d_x, nsize*sizeof(float));

  // execute kernel

  my_first_kernel<<<nblocks,nthreads>>>(d_x);

  // copy back results and print them out

  cudaMemcpy(h_x,d_x,nsize*sizeof(float),cudaMemcpyDeviceToHost);

  for (n=0; n<nsize; n++) printf(" n,  x  =  %d  %f \n",n,h_x[n]);

  // free memory

  cudaFree(d_x);
  free(h_x);

  return 0;
}
