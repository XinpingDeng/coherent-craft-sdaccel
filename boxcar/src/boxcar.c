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
  core_t boxcar;
  core_t previous;
  
  for(i = 0; i < ndm; i++){
    for(j = 0; j < NSAMP_PER_IMG; j++){
      // boxcar1
      for(m = 0; m < ntime; m++){
	loc_in  = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	loc_out = loc_in;	
	out1[loc_out] = in[loc_in];
      }

      // boxcar2
      boxcar = 0;
      loc_in = i*ntime*NSAMP_PER_IMG + j;
      previous = in[loc_in];
      for(m = 0; m < 2; m++){
	loc_in = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	boxcar += in[loc_in];	
      }
      loc_out      = i*(ntime-1)*NSAMP_PER_IMG + j;
      out2[loc_out] = boxcar;      
      for(m = 2; m < ntime; m++){
	loc_in   = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	boxcar   += in[loc_in];
	boxcar   -= previous;

	loc_out       = i*(ntime-1)*NSAMP_PER_IMG + (m-1)*NSAMP_PER_IMG + j;
	out2[loc_out] = boxcar;
	
	loc_in   = i*ntime*NSAMP_PER_IMG + (m-1)*NSAMP_PER_IMG + j;
	previous = in[loc_in];
      }
      
      // boxcar3
      boxcar = 0;
      loc_in = i*ntime*NSAMP_PER_IMG + j;
      previous = in[loc_in];
      for(m = 0; m < 3; m++){
	loc_in = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	boxcar += in[loc_in];	
      }
      loc_out      = i*(ntime-2)*NSAMP_PER_IMG + j;
      out3[loc_out] = boxcar;      
      for(m = 3; m < ntime; m++){
	loc_in   = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	boxcar   += in[loc_in];
	boxcar   -= previous;

	loc_out       = i*(ntime-2)*NSAMP_PER_IMG + (m-2)*NSAMP_PER_IMG + j;
	out3[loc_out] = boxcar;
	
	loc_in   = i*ntime*NSAMP_PER_IMG + (m-2)*NSAMP_PER_IMG + j;
	previous = in[loc_in];
      }
    }
  }
  return EXIT_SUCCESS;
}
