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
  int k;
  int loc;
  int loc_img;
  accum_t scale_factor[NBOXCAR];
  //real_t history[MAX_SMP_PER_IMG][NBOXCAR-1];
  //real_t history_tile[NBOXCAR];
  real_t history[MAX_BURST_PER_IMG][NREAL_PER_BURST*(NBOXCAR-1)];
  real_t history_tile[NBOXCAR][NREAL_PER_BURST];
  accum_t snr;
  accum_t scaled_snr;
  cand_t cand;
  int out_index = 0;
  int nburst_per_img = nsmp_per_img/NREAL_PER_BURST;
  
  FILE *fp = fopen("/data/FRIGG_2/Workspace/coherent-craft-sdaccel/boxcar/src/out_test.txt", "w");

  for(i = 0; i < NBOXCAR; i++){
    scale_factor[i] = hls::rsqrtf(i+1.0);
  }
  
  for(i = 0; i < ndm; i++){
    // Read in history
    for(m = 0; m < nburst_per_img; m++){
      for(n = 0; n < NBOXCAR-1; n++){
        for(j = 0; j < NREAL_PER_BURST; j++){
          loc = i*nburst_per_img*(NBOXCAR-1)*NREAL_PER_BURST
            + m*(NBOXCAR-1)*NREAL_PER_BURST
            + n*NREAL_PER_BURST
            + j;
          history[m][n*NREAL_PER_BURST+j] = history_in[loc];
        }
      }
    }

    // Do boxcar
    for(j = 0; j < ntime; j++){
      for(m = 0; m < nburst_per_img; m++){
        for(n = 0; n < NREAL_PER_BURST; n++){
          // Get current history tile ready
          loc = i*ntime*nburst_per_img*NREAL_PER_BURST
            + j*nburst_per_img*NREAL_PER_BURST
            + m*NREAL_PER_BURST
            + n;
          history_tile[0][n] = in[loc];
        }
        
        for(k = 1; k < NBOXCAR; k++){
          for(n = 0; n < NREAL_PER_BURST; n++){
            history_tile[k][n] = history[m][(k-1)*NREAL_PER_BURST+n];
          }
        }

        for(n = 0; n < NREAL_PER_BURST; n++){
          loc_img = m*NREAL_PER_BURST+n;
          
          //fprintf(fp, "HISTORY_TILE\t%f\n", history_tile[14][n].to_float());
          // The the maximun boxcar of each sample
          snr      = 0;
          cand.snr = 0;
          for(k = 0; k < NBOXCAR; k++){
            snr       += history_tile[k][n];
            scaled_snr = snr*scale_factor[k];
                        
            if(scaled_snr>cand.snr){
              fprintf(fp, "HISTORY_TILE %d\t%d\t%f\t%f\t%f\n", loc_img, k+1, history_tile[k][n].to_float(), snr.to_float(), scaled_snr.to_float());

              cand.snr          = scaled_snr;
              cand.loc_img      = loc_img;
              cand.boxcar_width = k+1;
              cand.time         = j;
              cand.dm           = i;
            }
          }
          
          // Write out candidate if the maximun is bigger than threshold
          if((cand.snr>=THRESHOLD) &&(out_index<MAX_CAND)){
            //fprintf(fp, "HISTORY_TILE %f\n", cand.snr.to_float());
            out[out_index] = cand;
            out_index++;
          }
        }
        
        // Backup current history_tile
        for(k = 0; k < NBOXCAR-1; k++){
          for(n = 0; n < NREAL_PER_BURST; n++){
            history[m][k*NREAL_PER_BURST+n] = history_tile[k][n];
          }
        }
      }
    }
    
    // Write history
    for(m = 0; m < nburst_per_img; m++){
      for(n = 0; n < NBOXCAR-1; n++){
        for(j = 0; j < NREAL_PER_BURST; j++){
          loc = i*nburst_per_img*(NBOXCAR-1)*NREAL_PER_BURST
            + m*(NBOXCAR-1)*NREAL_PER_BURST
            + n*NREAL_PER_BURST
            + j;
          history_out[loc] = history[m][n*NREAL_PER_BURST+j];
        }
      }
    }
  }
  
  fclose(fp);
  
  return EXIT_SUCCESS;  
}
