/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "boxcar.h"
#include "util_sdaccel.h"

int boxcar(
	   const core_t *in,
	   core_t *out1,
	   core_t *out2,
	   core_t *out3,
	   int ndm,
	   int ntime
	   ){
  int i;
  int j;
  int m;
  long loc_in;
  long loc_out;
  
  for(i = 0; i < ndm; i++){
    for(j = 0; j < ntime; j++){
      for(m = 0; m < NSAMP_PER_IMG; m++){
	loc_in  = i*ntime*NSAMP_PER_IMG + j*NSAMP_PER_IMG + m;
	loc_out = loc_in;

	out1[loc_out] = in[loc_in];
      }
    }
  }
}
