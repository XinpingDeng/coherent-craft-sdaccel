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
  long current_loc;
  long previous_loc;
  int tmp2;
  int tmp3;
  core_t current_in;
  core_t previous_in;
  
  for(i = 0; i < ndm; i++){
    for(j = 0; j < NSAMP_PER_IMG; j++){
      tmp2 = 0;
      tmp3 = 0;
      
      for(m = 0; m < ntime; m++){
	current_loc  = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	previous_loc = current_loc;
	current_in = in[current_loc];
	
	out1[current_loc] = current_in;
	
	if (m==0){
	  tmp2 = current_in;
	  tmp3 = current_in;
	}
	if(m==1){
	  previous_loc = i*ntime*NSAMP_PER_IMG + j;
	  tmp2 = tmp2 + current_in;
	  tmp3 = tmp3 + current_in;
	  
	  previous_in = in[previous_loc];
	  out2[previous_loc] = tmp2;
	}
	else{
	  tmp2 = tmp2 + current_in-previous_in;
	  
	}
      }
    }
  }
}
