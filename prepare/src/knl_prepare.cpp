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
    burst_data_type cal_pol1_burst[NBURST_PER_TIME_PER_BASELINE];	  	      
    burst_data_type cal_pol2_burst[NBURST_PER_TIME_PER_BASELINE];	  	  
    burst_data_type sky_burst[NBURST_PER_TIME_PER_BASELINE];
    burst_data_type out_burst;
    burst_data_type average_pol1_burst[NBURST_PER_TIME_PER_BASELINE];	       
    burst_data_type average_pol2_burst[NBURST_PER_TIME_PER_BASELINE];	  

    int i;
    int j;
    int m;
    int n;
    int loc;

  LOOP_NBASELINE:
    for(i = 0; i < NBASELINE; i++){
    LOOP_BURST_CAL_SKY_RESET_AVERAGE:
      for(j = 0; j < NBURST_PER_TIME_PER_BASELINE; j++){
#pragma HLS PIPELINE 
	loc = i*NBURST_PER_TIME_PER_BASELINE + j;
	cal_pol1_burst[j] = cal_pol1[loc];
	cal_pol2_burst[j] = cal_pol2[loc];
	sky_burst[j]      = sky[loc];
	
      LOOP_RESET_AVERAGE:
	for(m = 0; m < NDATA_PER_BURST; m++){
#pragma HLS UNROLL
	  average_pol1_burst[j].data[m] = 0;
	  average_pol2_burst[j].data[m] = 0;
	}
      }
      
    LOOP_NTIME_PER_CU:
      for(j = 0; j < NTIME_PER_CU; j++){	
      LOOP_CAL_AVERAGE_OUT_M:
	for(m = 0; m < NBURST_PER_TIME_PER_BASELINE; m++)
	  {
#pragma HLS PIPELINE
	    loc = j*NBASELINE*NBURST_PER_TIME_PER_BASELINE + i*NBURST_PER_TIME_PER_BASELINE + m;	  
	    in_pol1_burst = in_pol1[loc];
	    in_pol2_burst = in_pol2[loc];
	  LOOP_CAL_AVERAGE_OUT_N:
	    for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	      average_pol1_burst[m].data[2*n]   += in_pol1_burst.data[2*n];
	      average_pol1_burst[m].data[2*n+1] += in_pol1_burst.data[2*n+1];
	      average_pol2_burst[m].data[2*n]   += in_pol2_burst.data[2*n];
	      average_pol2_burst[m].data[2*n+1] += in_pol2_burst.data[2*n+1];
	      
	      out_burst.data[2*n] = in_pol1_burst.data[2*n]*cal_pol1_burst[m].data[2*n] - in_pol1_burst.data[2*n+1]*cal_pol1_burst[m].data[2*n+1] + 
		in_pol2_burst.data[2*n]*cal_pol2_burst[m].data[2*n] - in_pol2_burst.data[2*n+1]*cal_pol2_burst[m].data[2*n+1] - 
		sky_burst[m].data[2*n];
	      
	      out_burst.data[2*n+1] = in_pol1_burst.data[2*n]*cal_pol1_burst[m].data[2*n+1] + in_pol1_burst.data[2*n+1]*cal_pol1_burst[m].data[2*n] + 
		in_pol2_burst.data[2*n]*cal_pol2_burst[m].data[2*n+1] + in_pol2_burst.data[2*n+1]*cal_pol2_burst[m].data[2*n] - 
		sky_burst[m].data[2*n+1];
	    }
	    out[loc] = out_burst;
	  }
      }
      
    LOOP_BURST_AVERAGE:
      for(j = 0; j < NBURST_PER_TIME_PER_BASELINE; j++){
#pragma HLS PIPELINE 
	loc = i*NBURST_PER_TIME_PER_BASELINE + j;
	average_pol1[loc] = average_pol1_burst[j];
	average_pol2[loc] = average_pol2_burst[j];
      }      
    }
  }
}
