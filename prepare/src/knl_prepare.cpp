/*
******************************************************************************
** C PREPARE KERNEL
******************************************************************************
*/

#include "prepare.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {
  void knl_prepare(
		   const burst_data_2pol_type *in,  // Read-Only input raw data
		   const burst_data_2pol_type *cal, // Read-Only input calibration
		   const burst_data_1pol_type *sky, // Read-Only input sky model
		   burst_data_1pol_type *out,       // Output raw data
		   burst_data_2pol_type *average   // Output average data, lives with multiple kernel instances
		   ){
#pragma HLS INTERFACE m_axi port=in offset=slave bundle=gmem0 //max_read_burst_length=16
#pragma HLS INTERFACE m_axi port=cal offset=slave bundle=gmem1 //max_read_burst_length=16
#pragma HLS INTERFACE m_axi port=sky offset=slave bundle=gmem2 //max_read_burst_length=16
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem3 //max_write_burst_length=16
#pragma HLS INTERFACE m_axi port=average offset=slave bundle=gmem4 //max_write_burst_length=16
#pragma HLS INTERFACE s_axilite port=in  bundle=control
#pragma HLS INTERFACE s_axilite port=cal  bundle=control
#pragma HLS INTERFACE s_axilite port=sky bundle=control
#pragma HLS INTERFACE s_axilite port=out bundle=control
#pragma HLS INTERFACE s_axilite port=average bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
    
#pragma HLS DATA_PACK variable = in 
#pragma HLS DATA_PACK variable = cal
#pragma HLS DATA_PACK variable = sky
#pragma HLS DATA_PACK variable = out 
#pragma HLS DATA_PACK variable = average 

    burst_data_2pol_type in_burst;
    burst_data_2pol_type cal_burst;
    burst_data_1pol_type sky_burst;
    burst_data_1pol_type out_burst;
    burst_data_2pol_type average_burst;
    
    int i;
    int j;
    int k;
    int loc;
    
  LOOP_NBURST_PER_TIME:
    for(i = 0; i < NBURST_PER_TIME; i++){
      cal_burst = cal[i];
      sky_burst = sky[i];
      
    LOOP_RESET_AVERAGE:
      for(j = 0; j < NDATA_PER_BURST_2POL; j++){
#pragma HLS UNROLL
	average_burst.data[j] = 0;
      }
      
    LOOP_NTIME_PER_CU:
      for(j = 0; j < NTIME_PER_CU; j++){
#pragma HLS PIPELINE
	loc = j*NBURST_PER_TIME + i;
	in_burst = in[loc];
	
      LOOP_NSAMP_PER_BURST:
	for(k = 0; k < NSAMP_PER_BURST; k++){
#pragma HLS UNROLL
	  average_burst.data[4*k]   += in_burst.data[4*k];
	  average_burst.data[4*k+1] += in_burst.data[4*k+1];
	  average_burst.data[4*k+2] += in_burst.data[4*k+2];
	  average_burst.data[4*k+3] += in_burst.data[4*k+3];
	  
	  out_burst.data[2*k] = in_burst.data[4*k]*cal_burst.data[4*k]-in_burst.data[4*k+1]*cal_burst.data[4*k+1]+// Real of pol 1 after cal
	    in_burst.data[4*k+2]*cal_burst.data[4*k+2]-in_burst.data[4*k+3]*cal_burst.data[4*k+3]-// Real of pol 2 after cal
	    sky_burst.data[k*2];// Real of sky
	  
	  out_burst.data[2*k+1] = in_burst.data[4*k]*cal_burst.data[4*k+1]+in_burst.data[4*k+1]*cal_burst.data[4*k]+// Image of pol 1 after cal
	    in_burst.data[4*k+2]*cal_burst.data[4*k+3]+in_burst.data[4*k+3]*cal_burst.data[4*k+2]-// Image of pol 2 after cal
	    sky_burst.data[k*2+1];// Image of sky	
	}
	out[loc] = out_burst;
      }
      average[i] = average_burst;
    }
  }
}
