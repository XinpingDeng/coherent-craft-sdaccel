#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		burst_data_type *in,
		burst_data_type *coordinate,
		burst_data_type *out,
		int nburst_per_uv,
		int nuv_per_cu
		){
    int i;
    int j;
    int m;
    int loc_in_burst;
    int loc_uv_samp;
    int loc_out_burst;
    int loc_out_remind;
    
    burst_data_type in_burst;
    burst_data_type coordinate_burst;
    burst_data_type out_burst;

    for(i = 0; i < nuv_per_cu; i++){
      for(j = 0; j < nburst_per_uv; j++){
#pragma HLS PIPELINE
	loc_in_burst     = i*nburst_per_uv + j;
	in_burst         = in[loc_in_burst];
	coordinate_burst = coordinate[j];
	
	for(m = 0; m < NSAMP_PER_BURST; m++){
#pragma HLS UNROLL
	  loc_uv_samp    = coordinate_burst.data[2*m] * FFT_SIZE + coordinate_burst.data[2*m+1];
	  loc_out_burst  = loc_uv_samp/NSAMP_PER_BURST;
	  loc_out_remind = loc_uv_samp%NSAMP_PER_BURST;
	  
	  //out[loc_out_burst].data[loc_out_remind] = in_burst.data[2*m];
	  //out[loc_out_burst].data[loc_out_remind] = in_burst.data[2*m+j];
	}
      }
    }
  }
}
