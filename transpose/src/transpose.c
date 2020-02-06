/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "transpose.h"
#include "util_sdaccel.h"

int transpose(
              uv_data_t *in,
              uv_data_t *out,
              int nsamp_per_uv_out,
              int ntime_per_cu,
              int ndm_per_cu){
  int i;
  int j;
  int k;
  int loc_in;
  int loc_out;
  
  for(i = 0; i < ntime_per_cu; i++){
    for(j = 0; j < nsamp_per_uv_out; j++){
      for(k = 0; k < ndm_per_cu; k++){
        loc_in  = j*ntime_per_cu*ndm_per_cu + i * ndm_per_cu + k;
        loc_out = k*nsamp_per_uv_out*ntime_per_cu + i * nsamp_per_uv_out + j;
        
        out[2*loc_out]   = in[2*loc_in];
        out[2*loc_out+1] = in[2*loc_in+1];
      }        
    }
  }
  
  return EXIT_SUCCESS;
}
