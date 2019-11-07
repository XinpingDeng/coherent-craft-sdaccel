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
#pragma HLS INTERFACE m_axi port = in_pol1 offset = slave bundle = gmem0 max_read_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = in_pol2 offset = slave bundle = gmem1 max_read_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = cal_pol1 offset = slave bundle = gmem2 max_read_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = cal_pol2 offset = slave bundle = gmem3 max_read_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = sky offset = slave bundle = gmem4 max_read_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem5 max_write_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = average_pol1 offset = slave bundle = gmem6 max_write_burst_length=64 depth=64
#pragma HLS INTERFACE m_axi port = average_pol2 offset = slave bundle = gmem7 max_write_burst_length=64 depth=64

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
    burst_data_type cal_pol1_burst[MAX_BURST_LENGTH];	  	      
    burst_data_type cal_pol2_burst[MAX_BURST_LENGTH];	  	  
    burst_data_type sky_burst[MAX_BURST_LENGTH];
    burst_data_type out_burst;
    burst_data_type average_pol1_burst[MAX_BURST_LENGTH];	       
    burst_data_type average_pol2_burst[MAX_BURST_LENGTH];
        
    int i;
    int j;
    int m;
    int n;
    int loc;
    int itran;
    
  LOOP_BURST_CAL_SKY_RESET_AVERAGE1:
    for(i = 0; i < MAX_BURST_LENGTH; i++){
#pragma HLS PIPELINE 
      cal_pol1_burst[i] = cal_pol1[i];
      cal_pol2_burst[i] = cal_pol2[i];
      sky_burst[i]      = sky[i];
      
    LOOP_RESET_AVERAGE1:
      for(j = 0; j < NDATA_PER_BURST; j++){
#pragma HLS UNROLL
	average_pol1_burst[i].data[j] = 0;
	average_pol2_burst[i].data[j] = 0;
      }	  
    }
    
  LOOP_NTRAN_PER_TIME:
    for(i = 0; i < NTRAN_PER_TIME - 1; i++){
    LOOP_NTIME_PER_CU1:
      for(j = 0; j < NTIME_PER_CU; j++){	
      LOOP_CAL_AVERAGE_OUT_M1:
	for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE
	  loc = j*NBURST_PER_TIME + i*MAX_BURST_LENGTH + m; 
	  in_pol1_burst = in_pol1[loc];
	  in_pol2_burst = in_pol2[loc];
	LOOP_CAL_AVERAGE_OUT_N1:
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
      
    LOOP_BURST_AVERAGE1:
      for(j = 0; j < MAX_BURST_LENGTH; j++){
#pragma HLS PIPELINE 
	loc = i*MAX_BURST_LENGTH + j;
	average_pol1[loc] = average_pol1_burst[j];
	average_pol2[loc] = average_pol2_burst[j];
      	
	loc += MAX_BURST_LENGTH;
	cal_pol1_burst[j] = cal_pol1[loc];
	cal_pol2_burst[j] = cal_pol2[loc];
	sky_burst[j]      = sky[loc];
	
      LOOP_RESET_AVERAGE2:
	for(m = 0; m < NDATA_PER_BURST; m++){
#pragma HLS UNROLL
	  average_pol1_burst[j].data[m] = 0;
	  average_pol2_burst[j].data[m] = 0;
	}
      }      
    }

    //////////////////////////////////////////
    itran = NTRAN_PER_TIME - 1;
  LOOP_NTIME_PER_CU2:
    for(j = 0; j < NTIME_PER_CU; j++){	
    LOOP_CAL_AVERAGE_OUT_M2:
      for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE
	loc = j*NBURST_PER_TIME + itran*MAX_BURST_LENGTH + m; 
	in_pol1_burst = in_pol1[loc];
	in_pol2_burst = in_pol2[loc];
      LOOP_CAL_AVERAGE_OUT_N2:
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
    
  LOOP_BURST_AVERAGE2:
    for(j = 0; j < MAX_BURST_LENGTH; j++){
#pragma HLS PIPELINE 
      loc = itran*MAX_BURST_LENGTH + j;
      average_pol1[loc] = average_pol1_burst[j];
      average_pol2[loc] = average_pol2_burst[j];
    }
    
  LOOP_BURST_CAL_SKY_RESET_AVERAGE2:
    for(j = 0; j < NBURST_PER_TIME_REMIND; j++){    
      loc = (itran+1)*MAX_BURST_LENGTH+j;
      cal_pol1_burst[j] = cal_pol1[loc];
      cal_pol2_burst[j] = cal_pol2[loc];
      sky_burst[j]      = sky[loc];
      
    LOOP_RESET_AVERAGE3:
      for(m = 0; m < NDATA_PER_BURST; m++){
#pragma HLS UNROLL
	average_pol1_burst[j].data[m] = 0;
	average_pol2_burst[j].data[m] = 0;
      }
    }
    
    //////////////////////////
    itran = NTRAN_PER_TIME;
  LOOP_NTIME_PER_CU3:
    for(j = 0; j < NTIME_PER_CU; j++){	
    LOOP_CAL_AVERAGE_OUT_M3:
      for(m = 0; m < NBURST_PER_TIME_REMIND; m++){
#pragma HLS PIPELINE
	loc = j*NBURST_PER_TIME + itran*MAX_BURST_LENGTH + m; 
	in_pol1_burst = in_pol1[loc];
	in_pol2_burst = in_pol2[loc];
      LOOP_CAL_AVERAGE_OUT_N3:
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
    
  LOOP_BURST_AVERAGE3:
    for(j = 0; j < NBURST_PER_TIME_REMIND; j++){
#pragma HLS PIPELINE 
      loc = itran*MAX_BURST_LENGTH + j;
      average_pol1[loc] = average_pol1_burst[j];
      average_pol2[loc] = average_pol2_burst[j];
    }    
    ///////////////////////////////////////////////
  }
}
