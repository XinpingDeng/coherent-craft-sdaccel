/*
******************************************************************************
** C TEST BENCH FOR PREPARE KERNEL
******************************************************************************
*/

#include "util_sdaccel.h"
#include "prepare.h"

int prepare(complex_data_type *in, complex_data_type *cal, complex_data_type *sky, complex_data_type *out, complex_data_type *average)
{
  int i;
  int j;
  int loc;

  for(i = 0; i < NSAMP_PER_TIME; i++){      
    for(j = 0; j < NTIME_PER_CU; j++){
      loc = j*NSAMP_PER_TIME + i;
            
      average[2*i]   += in[2*loc];
      average[2*i+1] += in[2*loc+1];
	
      out[loc] = in[loc*2] * cal[2*i] +
	in[loc*2+1] * cal[2*i+1] -
	sky[i];
      }
  }
  
  return EXIT_SUCCESS;
}
