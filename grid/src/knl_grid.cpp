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
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 //max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 //max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 //max_read_burst_length=64

#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = out        bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
    
#pragma HLS INTERFACE s_axilite port = return bundle = control

  int i;
  int j;
  int m;
  int loc_in_burst;
  int loc_out_burst;
  int loc_out;
  burst_uv in_burst;
  burst_uv out_burst[NSAMP_PER_BURST];
  burst_coord coord_burst;

#pragma HLS DATA_PACK variable = in_burst
#pragma HLS DATA_PACK variable = out_burst
#pragma HLS DATA_PACK variable = coord_burst
    
 LOOP_I:
  for(i = 0; i < nuv_per_cu; i++){
  LOOP_J:
    for(j = 0; j < NBURST_PER_UV_IN; j++){
#pragma HLS PIPELINE
      loc_in_burst = i*NBURST_PER_UV_IN + j;
      in_burst     = in[loc_in_burst];
      coord_burst  = coord[loc_in_burst];

      for(m = 0; m < NSAMP_PER_BURST; m++){
#pragma HLS UNROLL
	loc_out = i*NSAMP_PER_UV_OUT +
	  coord_burst.data[2*m]*FFT_SIZE +
	  coord_burst.data[2*m+1];
	  
	out_burst[m].data[2*(loc_out%NSAMP_PER_BURST)]     = in_burst.data[2*m];
	out_burst[m].data[2*(loc_out%NSAMP_PER_BURST) + 1] = in_burst.data[2*m + 1];
      }
    }
  }
}
