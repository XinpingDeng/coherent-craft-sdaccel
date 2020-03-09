/*
******************************************************************************
** BOXCAR CODE FILE
******************************************************************************
*/

#include "boxcar.h"
#include "util_sdaccel.h"

int boxcar(
	   const real_t *in,
           int ndm,
           int ntime,
           int nsmp_per_img,
           real_t *history_in,
           real_t *history_out,
           cand_t *out
	   ){
  int i;
  int j;
  int m;
  int n;
  int loc;
  real_t history[MAX_SMP_PER_IMG][NBOXCAR-1];
  real_t history_tile[NBOXCAR];
  accum_t boxcar;
  accum_t scaled_boxcar;
  cand_t cand;
  int out_index = 0;
  
  for(i = 0; i < ndm; i++){
    // Read in history
    for(m = 0; m < nsmp_per_img; m++){
      for(n = 0; n < NBOXCAR-1; n++){
        loc = i*nsmp_per_img*(NBOXCAR-1)
          + m*(NBOXCAR-1)
          + n;
        history[m][n] = history_in[loc];
      }
    }

    // Do boxcar
    for(j = 0; j < ntime; j++){
      for(m = 0; m < nsmp_per_img; m++){
        // Get current history tile ready
        loc = i*ntime*nsmp_per_img
          + j*nsmp_per_img
          + m;
        history_tile[0] = in[loc];
        for(n = 1; n < NBOXCAR; n++){
          history_tile[n] = history[m][n-1];
        }

        // The the maximun boxcar of each sample
        boxcar   = 0;
        cand.snr = 0;
        for(n = 0; n < NBOXCAR; n++){
          boxcar       += history_tile[n];
          scaled_boxcar = boxcar*(accum_t)hls::rsqrtf(n+1);
          if(scaled_boxcar>cand.snr){
            cand.snr          = scaled_boxcar;
            cand.loc_img      = loc;
            cand.boxcar_width = n+1;
            cand.time         = j;
            cand.dm           = i;
          }
        }

        // Write out candidate if the maximun is bigger than threshold
        if((cand.snr>=THRESHOLD) &&(out_index<MAX_CAND)){
          out[out_index] = cand;
          out_index++;
        }
        
        // Backup current history_tile
        for(n = 0; n < NBOXCAR-1; n++){
          history[m][n] = history_tile[n]; 
        }
      }
    }
    
    // Write history
    for(m = 0; m < nsmp_per_img; m++){
      for(n = 0; n < NBOXCAR-1; n++){
        loc = i*nsmp_per_img*(NBOXCAR-1)
          + m*(NBOXCAR-1)
          + n;
        history_out[loc] = history[m][n];
      }
    }
  }
  
  return EXIT_SUCCESS;  
}
