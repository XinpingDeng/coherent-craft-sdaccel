#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		const burst_data_type *in,
		const burst_data_type *coordinate,
		burst_data_type *out,
		int nuv_per_cu
		){
#pragma HLS INTERFACE m_axi port = in         offset = slave bundle = gmem0 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = coordinate offset = slave bundle = gmem1 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = out        offset = slave bundle = gmem2 max_read_burst_length=64

#pragma HLS INTERFACE s_axilite port = in           bundle = control
#pragma HLS INTERFACE s_axilite port = coordinate   bundle = control
#pragma HLS INTERFACE s_axilite port = out          bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu   bundle = control
    
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = coordinate
#pragma HLS DATA_PACK variable = out
    
    int i;
    int j;
    int m;
    int loc_burst_in;
    int loc_burst_out;
    int loc_burst_out_ref = 0;
    int loc_in_remind;
    int loc_out_remind;
    int loc_out;
    
    burst_data_type in_burst;
    burst_data_type coordinate_burst;
    burst_data_type out_burst;

    // Init out burst 
    for(m = 0; m < NDATA_PER_BURST; m++){
#pragma HLS UNROLL
      out_burst.data[m] = 0;
    }
    
  LOOP_I:
    for(i = 0; i < nuv_per_cu; i++){
    LOOP_J:
      for(j = 0; j < NSAMP_PER_UV_IN; j++){
#pragma HLS PIPELINE
	// Read in data every NSAMP_PER_BURST clock cycles, ideally;
      COND_REGION: {
	  //#pragma HLS OCCURRENCE cycle = NSAMP_PER_BURST
#pragma HLS OCCURRENCE cycle = 16 // NSAMP_PER_BURST
	  
	  loc_burst_in     = (i*NSAMP_PER_UV_IN + j)/NSAMP_PER_BURST;
	  in_burst         = in[loc_burst_in];
	  coordinate_burst = coordinate[j/NSAMP_PER_BURST];
	}
	
	loc_in_remind  = j%NSAMP_PER_BURST; // Assume that NSAMP_PER_UV_IN%NSAMP_PER_BURST = 0, pad zeros if necessary
	loc_out        = coordinate_burst.data[2*loc_in_remind]*FFT_SIZE + coordinate_burst.data[2*loc_in_remind+1];
	loc_burst_out  = loc_out/NSAMP_PER_BURST;

	if(loc_burst_out!=loc_burst_out_ref){
	  out[loc_burst_out] = out_burst;
	  loc_burst_out_ref = loc_burst_out;
	}
	
	// Fill the write burst with ture value
	loc_out_remind = loc_out%NSAMP_PER_BURST;
	out_burst.data[2*loc_out_remind]   = in_burst.data[2*loc_in_remind];
	out_burst.data[2*loc_out_remind+1] = in_burst.data[2*loc_in_remind+1];
      }
    }
  }
}
