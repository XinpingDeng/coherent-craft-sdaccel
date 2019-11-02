#include "prepare.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {
  void knl_prepare(
		   burst_data_type *in_pol1,
		   burst_data_type *in_pol2,  
		   burst_data_type *cal_pol1,
		   burst_data_type *cal_pol2, 
		   burst_data_type *sky,
		   burst_data_type *out,       
		   burst_data_type *average_pol1,
		   burst_data_type *average_pol2   
		   ){
#pragma HLS INTERFACE m_axi port = in_pol1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in_pol2 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = cal_pol1 offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = cal_pol2 offset = slave bundle = gmem3
#pragma HLS INTERFACE m_axi port = sky offset = slave bundle = gmem4
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem5
#pragma HLS INTERFACE m_axi port = average_pol1 offset = slave bundle = gmem6
#pragma HLS INTERFACE m_axi port = average_pol2 offset = slave bundle = gmem7

#pragma HLS INTERFACE s_axilite port = in_pol1 bundle = control
#pragma HLS INTERFACE s_axilite port = in_pol2 bundle = control
#pragma HLS INTERFACE s_axilite port = cal_pol1 bundle = control
#pragma HLS INTERFACE s_axilite port = cal_pol2 bundle = control
#pragma HLS INTERFACE s_axilite port = sky bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = average_pol1 bundle = control
#pragma HLS INTERFACE s_axilite port = average_pol2 bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control
    
#pragma HLS DATA_PACK variable = in_pol1
#pragma HLS DATA_PACK variable = in_pol2
#pragma HLS DATA_PACK variable = cal_pol1
#pragma HLS DATA_PACK variable = cal_pol2
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = sky
#pragma HLS DATA_PACK variable = average_pol1
#pragma HLS DATA_PACK variable = average_pol2

    burst_data_type in_pol1_burst;	  
    burst_data_type in_pol2_burst;	  
    burst_data_type cal_pol1_burst;	      
    burst_data_type cal_pol2_burst;	  
    burst_data_type sky_burst;
    burst_data_type out_burst;       	      
    burst_data_type average_pol1_burst;     
    burst_data_type average_pol2_burst;

    int i;
    int j;
    int k;
    int loc;
    
  LOOP_COMPUTE_OUT_AVERAGE_STREAM:
    for(i = 0; i < NBURST_PER_TIME; i++){
      cal_pol1_burst = cal_pol1[i] ;
      cal_pol2_burst = cal_pol2[i] ;
      sky_burst      = sky[i] ;
      
    LOOP_NSAMP_PER_BURST1:
      for(k = 0; k < NDATA_PER_BURST; k++){
#pragma HLS UNROLL
	average_pol1_burst.data[k]   = 0;
	average_pol2_burst.data[k]   = 0;
      }	  
      
    LOOP_NTIME_PER_CU:
      for(j = 0; j < NTIME_PER_CU; j++){
#pragma HLS PIPELINE 
	loc = j*NBURST_PER_TIME + i;
	
	in_pol1_burst = in_pol1[loc] ;
	in_pol2_burst = in_pol2[loc] ;
	
      LOOP_NSAMP_PER_BURST2:
	for(k = 0; k < NSAMP_PER_BURST; k++){
#pragma HLS UNROLL
	  average_pol1_burst.data[2*k]   += in_pol1_burst.data[2*k];
	  average_pol1_burst.data[2*k+1] += in_pol1_burst.data[2*k+1];
	  average_pol2_burst.data[2*k]   += in_pol2_burst.data[2*k];
	  average_pol2_burst.data[2*k+1] += in_pol2_burst.data[2*k+1];
	
	  out_burst.data[2*k] = in_pol1_burst.data[2*k]*cal_pol1_burst.data[2*k] - in_pol1_burst.data[2*k+1]*cal_pol1_burst.data[2*k+1] + // Real of pol 1 after cal
	    in_pol2_burst.data[2*k]*cal_pol2_burst.data[2*k] - in_pol2_burst.data[2*k+1]*cal_pol2_burst.data[2*k+1] - // Real of pol 2 after cal
	    sky_burst.data[2*k];// Real of sky
	
	  out_burst.data[2*k+1] = in_pol1_burst.data[2*k]*cal_pol1_burst.data[2*k+1] + in_pol1_burst.data[2*k+1]*cal_pol1_burst.data[2*k] + // Image of pol 1 after cal
	    in_pol2_burst.data[2*k]*cal_pol2_burst.data[2*k+1] + in_pol2_burst.data[2*k+1]*cal_pol2_burst.data[2*k] - // Image of pol 2 after cal
	    sky_burst.data[2*k+1];// Image of sky	  
	}
      
	out[loc] = out_burst;
      }
      
      average_pol1[i] = average_pol1_burst;
      average_pol2[i] = average_pol2_burst;
    }
  }
}
