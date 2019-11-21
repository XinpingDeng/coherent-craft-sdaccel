#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		const burst_uv *in,
		const burst_coord *coord,
		burst_uv *out,
		int nuv_per_cu
		);
}

void knl_grid(
	      const burst_uv *in,
	      const burst_coord *coord,
	      burst_uv *out,
	      int nuv_per_cu
	      )
{
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_read_burst_length=64

#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = out        bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
    
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = coord
  
  burst_coord coord_burst;
  burst_uv in_burst[NBURST_PER_UV_IN];
  burst_uv out_burst;
  
  #pragma HLS DATA_PACK variable = in_burst
  #pragma HLS DATA_PACK variable = out_burst
  #pragma HLS DATA_PACK variable = coord_burst

#pragma HLS ARRAY_PARTITION variable = in_burst  
  
    int i;
    int j;
    int m;
    int loc_uv_in;
    int loc_in;
    int loc_out;
    int loc_burst;
    int loc_samp;
    uv_t tmp1;
    uv_t tmp2;
    
 LOOP_SET_UV_TOP:
    for(i = 0; i < nuv_per_cu; i++){
    LOOP_BURST_UV_IN:
      // Burst in UV_IN
      for(j = 0; j < NBURST_PER_UV_IN; j++){
#pragma HLS PIPELINE
	loc_in = i*NBURST_PER_UV_IN + j;
	in_burst[j] = in[loc_in];
      }
    
    LOOP_SET_UV:
      for(j = 0; j < NBURST_PER_UV_OUT; j++){
#pragma HLS PIPELINE
	coord_burst = coord[j];
	for(m = 0; m < NSAMP_PER_BURST; m++){
#pragma HLS UNROLL
	  loc_uv_in = coord_burst.data[2*m]-1;
	  if(loc_uv_in > -1){ // Means there is data
	    loc_burst = loc_uv_in/NSAMP_PER_BURST;
	    loc_samp  = loc_uv_in%NSAMP_PER_BURST;
	    
	    tmp1 = in_burst[loc_burst].data[2*loc_samp];
	    tmp2 = in_burst[loc_burst].data[2*loc_samp + 1];
	  }
	  else{// Means there is no data
	    tmp1 = 0;
	    tmp2 = 0;	    
	  }
	  out_burst.data[2*m]     = tmp1;
	  out_burst.data[2*m + 1] = tmp2;
	}
	
	loc_out = i*NBURST_PER_UV_OUT + j;
	out[loc_out] = out_burst;
      }    
    }  
}
