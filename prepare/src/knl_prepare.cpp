#include "prepare.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {
  void knl_prepare(
		   const burst_data_type *in_pol1,
		   const burst_data_type *in_pol2,  
		   const burst_data_type *cal_pol1,
		   const burst_data_type *cal_pol2, 
		   const burst_data_type *sky,
		   burst_data_type *out,       
		   burst_data_type *average_pol1,
		   burst_data_type *average_pol2,
		   int nburst_per_time,
		   int ntime_per_cu
		   );
}

void knl_prepare(
		 const burst_data_type *in_pol1,
		 const burst_data_type *in_pol2,  
		 const burst_data_type *cal_pol1,
		 const burst_data_type *cal_pol2, 
		 const burst_data_type *sky,
		 burst_data_type *out,       
		 burst_data_type *average_pol1,
		 burst_data_type *average_pol2,
		 int nburst_per_time,
		 int ntime_per_cu
		 )
{
  const int max_burst_length = MAX_BURST_LENGTH;
  // Setup the interface, max_*_burst_length defines the max burst length (UG902 for detail)
#pragma HLS INTERFACE m_axi port = in_pol1      offset = slave bundle = gmem0 max_read_burst_length=max_burst_length  
#pragma HLS INTERFACE m_axi port = in_pol2      offset = slave bundle = gmem1 max_read_burst_length=max_burst_length  
#pragma HLS INTERFACE m_axi port = cal_pol1     offset = slave bundle = gmem2 max_read_burst_length=max_burst_length  
#pragma HLS INTERFACE m_axi port = cal_pol2     offset = slave bundle = gmem3 max_read_burst_length=max_burst_length  
#pragma HLS INTERFACE m_axi port = sky          offset = slave bundle = gmem4 max_read_burst_length=max_burst_length  
#pragma HLS INTERFACE m_axi port = out          offset = slave bundle = gmem5 max_write_burst_length=max_burst_length 
#pragma HLS INTERFACE m_axi port = average_pol1 offset = slave bundle = gmem6 max_write_burst_length=max_burst_length 
#pragma HLS INTERFACE m_axi port = average_pol2 offset = slave bundle = gmem7 max_write_burst_length=max_burst_length 

#pragma HLS INTERFACE s_axilite port = in_pol1         bundle = control
#pragma HLS INTERFACE s_axilite port = in_pol2         bundle = control
#pragma HLS INTERFACE s_axilite port = cal_pol1        bundle = control
#pragma HLS INTERFACE s_axilite port = cal_pol2        bundle = control
#pragma HLS INTERFACE s_axilite port = sky             bundle = control
#pragma HLS INTERFACE s_axilite port = out             bundle = control
#pragma HLS INTERFACE s_axilite port = average_pol1    bundle = control
#pragma HLS INTERFACE s_axilite port = average_pol2    bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_time bundle = control
#pragma HLS INTERFACE s_axilite port = ntime_per_cu    bundle = control
    
#pragma HLS INTERFACE s_axilite port = return bundle = control
    
  // Has to use DATA_PACK to enable burst with struct
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
  burst_data_type out_burst;
  core_data_type cal_pol1_burst[MAX_BURST_LENGTH*NDATA_PER_BURST];	  	      
  core_data_type cal_pol2_burst[MAX_BURST_LENGTH*NDATA_PER_BURST];
  core_data_type sky_burst[MAX_BURST_LENGTH*NDATA_PER_BURST];  
  burst_data_type average_pol1_burst[MAX_BURST_LENGTH]; // Change this to a pure array does not help much
  burst_data_type average_pol2_burst[MAX_BURST_LENGTH]; // Change this to a pure array does not help much
        
  int i;
  int j;
  int m;
  int n;
  int loc;
  int loc_burst;
  int itran;
  int ntran_per_time;
  int nburst_per_time_remind;

  ntran_per_time         = nburst_per_time/MAX_BURST_LENGTH;
  nburst_per_time_remind = nburst_per_time%MAX_BURST_LENGTH;

  const int ndata_per_burst = NDATA_PER_BURST;
#pragma HLS ARRAY_RESHAPE variable=sky_burst cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal_pol1_burst cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal_pol2_burst cyclic factor=ndata_per_burst
  
 LOOP_BURST_CAL_SKY_RESET_AVERAGE1:
  for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE
    loc_burst = m;
      
  LOOP_RESET_AVERAGE1:
    for(n = 0; n < NDATA_PER_BURST; n++){
#pragma HLS UNROLL
      loc = m*NDATA_PER_BURST+n;
      sky_burst[loc] = sky[loc_burst].data[n];
      cal_pol1_burst[loc] = cal_pol1[loc_burst].data[n];
      cal_pol2_burst[loc] = cal_pol2[loc_burst].data[n];
      
      average_pol1_burst[m].data[n] = 0;
      average_pol2_burst[m].data[n] = 0;    
    }	  
  }

 LOOP_NTRAN_PER_TIME:
  for(i = 0; i < ntran_per_time - 1; i++){
#pragma HLS LOOP_TRIPCOUNT min=7830 max=7830
    // This goes to the last second if NCHAN*NBASELINE%MAX_BURST_LENGTH = 0
    // This goes to the last third if NCHAN*NBASELINE%MAX_BURST_LENGTH != 0
  LOOP_NTIME_PER_CU1:
    for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    LOOP_CAL_AVERAGE_OUT_M1:
      for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE
	loc_burst = j*nburst_per_time + i*MAX_BURST_LENGTH + m; 
	in_pol1_burst = in_pol1[loc_burst];
	in_pol2_burst = in_pol2[loc_burst];
      LOOP_CAL_AVERAGE_OUT_N1:
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  // UNROLL here requires multiple memory ports, caution
	  average_pol1_burst[m].data[2*n]   += in_pol1_burst.data[2*n];
	  average_pol1_burst[m].data[2*n+1] += in_pol1_burst.data[2*n+1];
	  average_pol2_burst[m].data[2*n]   += in_pol2_burst.data[2*n];
	  average_pol2_burst[m].data[2*n+1] += in_pol2_burst.data[2*n+1];
	  //loc = 2*(m*NDATA_PER_BURST+n);
	  loc = m*NDATA_PER_BURST+2*n;
	  
	  out_burst.data[2*n] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc] - in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc+1] + 
	    in_pol2_burst.data[2*n]*cal_pol2_burst[loc] - in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc+1] - 
	    sky_burst[loc];
	      
	  out_burst.data[2*n+1] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc+1] + in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc] + 
	    in_pol2_burst.data[2*n]*cal_pol2_burst[loc+1] + in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc] - 
	    sky_burst[loc+1];
	}
	out[loc_burst] = out_burst;
      }
    }
      
  LOOP_BURST_AVERAGE1:
    for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE 
      loc_burst = i*MAX_BURST_LENGTH + m;
      average_pol1[loc_burst] = average_pol1_burst[m];
      average_pol2[loc_burst] = average_pol2_burst[m];
      	
      loc_burst += MAX_BURST_LENGTH;
    LOOP_RESET_AVERAGE2:
      for(n = 0; n < NDATA_PER_BURST; n++){
#pragma HLS UNROLL
	loc = m*NDATA_PER_BURST+n;
	sky_burst[loc] = sky[loc_burst].data[n];
	cal_pol1_burst[loc] = cal_pol1[loc_burst].data[n];
	cal_pol2_burst[loc] = cal_pol2[loc_burst].data[n];   
	average_pol1_burst[m].data[n] = 0;
	average_pol2_burst[m].data[n] = 0;  
      }
    }      
  }

  //////////////////////////////////////////
  // This goes to the last if NCHAN*NBASELINE%MAX_BURST_LENGTH = 0
  // This goes to the last second if NCHAN*NBASELINE%MAX_BURST_LENGTH != 0
  itran = ntran_per_time - 1;
 LOOP_NTIME_PER_CU2:
  for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
  LOOP_CAL_AVERAGE_OUT_M2:
    for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE
      loc_burst = j*nburst_per_time + itran*MAX_BURST_LENGTH + m; 
      in_pol1_burst = in_pol1[loc_burst];
      in_pol2_burst = in_pol2[loc_burst];
    LOOP_CAL_AVERAGE_OUT_N2:
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	// UNROLL here requires multiple memory ports, caution
	average_pol1_burst[m].data[2*n]   += in_pol1_burst.data[2*n];
	average_pol1_burst[m].data[2*n+1] += in_pol1_burst.data[2*n+1];
	average_pol2_burst[m].data[2*n]   += in_pol2_burst.data[2*n];
	average_pol2_burst[m].data[2*n+1] += in_pol2_burst.data[2*n+1];
	//loc = 2*(m*NDATA_PER_BURST+n);
	loc = m*NDATA_PER_BURST+2*n;
	
	out_burst.data[2*n] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc] - in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc+1] + 
	  in_pol2_burst.data[2*n]*cal_pol2_burst[loc] - in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc+1] - 
	  sky_burst[loc];
      
	out_burst.data[2*n+1] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc+1] + in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc] + 
	  in_pol2_burst.data[2*n]*cal_pol2_burst[loc+1] + in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc] - 
	  sky_burst[loc+1];
      }
      out[loc_burst] = out_burst;
    }
  }
    
 LOOP_BURST_AVERAGE2:
  for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE 
    loc_burst = itran*MAX_BURST_LENGTH + m;
    average_pol1[loc_burst] = average_pol1_burst[m];
    average_pol2[loc_burst] = average_pol2_burst[m];
  }
    
 LOOP_BURST_CAL_SKY_RESET_AVERAGE2:
  for(m = 0; m < nburst_per_time_remind; m++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=max_burst_length
#pragma HLS PIPELINE 
    // Be sure that only go through and prepare for the reminding samples 
    loc_burst = (itran+1)*MAX_BURST_LENGTH+m;      
  LOOP_RESET_AVERAGE3:
    for(n = 0; n < NDATA_PER_BURST; n++){
#pragma HLS UNROLL
      loc = m*NDATA_PER_BURST+n;
      sky_burst[loc] = sky[loc_burst].data[n];
      cal_pol1_burst[loc] = cal_pol1[loc_burst].data[n];
      cal_pol2_burst[loc] = cal_pol2[loc_burst].data[n];   
      average_pol1_burst[m].data[n] = 0;
      average_pol2_burst[m].data[n] = 0;  
    }
  }
    
  //////////////////////////    
  // This goes to the last if NCHAN*NBASELINE%MAX_BURST_LENGTH != 0
  itran = ntran_per_time;
 LOOP_NTIME_PER_CU3:
  for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
  LOOP_CAL_AVERAGE_OUT_M3:
    for(m = 0; m < nburst_per_time_remind; m++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=max_burst_length
#pragma HLS PIPELINE
      loc_burst = j*nburst_per_time + itran*MAX_BURST_LENGTH + m; 
      in_pol1_burst = in_pol1[loc_burst];
      in_pol2_burst = in_pol2[loc_burst];
    LOOP_CAL_AVERAGE_OUT_N3:
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	// UNROLL here requires multiple memory ports, caution
	average_pol1_burst[m].data[2*n]   += in_pol1_burst.data[2*n];
	average_pol1_burst[m].data[2*n+1] += in_pol1_burst.data[2*n+1];
	average_pol2_burst[m].data[2*n]   += in_pol2_burst.data[2*n];
	average_pol2_burst[m].data[2*n+1] += in_pol2_burst.data[2*n+1];
	//loc = 2*(m*NDATA_PER_BURST+n);
	loc = m*NDATA_PER_BURST+2*n;
	
	out_burst.data[2*n] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc] - in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc+1] + 
	  in_pol2_burst.data[2*n]*cal_pol2_burst[loc] - in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc+1] - 
	  sky_burst[loc];
	  
	out_burst.data[2*n+1] = in_pol1_burst.data[2*n]*cal_pol1_burst[loc+1] + in_pol1_burst.data[2*n+1]*cal_pol1_burst[loc] + 
	  in_pol2_burst.data[2*n]*cal_pol2_burst[loc+1] + in_pol2_burst.data[2*n+1]*cal_pol2_burst[loc] - 
	  sky_burst[loc+1];
      }
      out[loc_burst] = out_burst;
    }
  }
    
 LOOP_BURST_AVERAGE3:
  for(m = 0; m < nburst_per_time_remind; m++){
    // only average out here
#pragma HLS LOOP_TRIPCOUNT min=1 max=max_burst_length
#pragma HLS PIPELINE 
    loc_burst = itran*MAX_BURST_LENGTH + m;
    average_pol1[loc_burst] = average_pol1_burst[m];
    average_pol2[loc_burst] = average_pol2_burst[m];
  }
  ///////////////////////////////////////////////
}
