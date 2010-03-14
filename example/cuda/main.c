
/* originally from http://people.maths.ox.ac.uk/gilesm/ and modified */

//
// include files
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//
// declare external routine
//
#include "prac5.h"

//
// main code
//

int
main (int argc, char **argv)
{
  // set number of blocks, and threads per block

  int                 nblocks = 2;
  int                 nthreads = 8;

  // call CUDA routine

  prac5 (nblocks, nthreads);

  return 0;
}
