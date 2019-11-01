#include "prepare.h"

void read_cal_sky(
		  burst_data_type *cal_pol1,
		  burst_data_type *cal_pol2,
		  burst_data_type *sky,
		  fifo_data_type &cal_pol1_fifo,
		  fifo_data_type &cal_pol2_fifo,
		  fifo_data_type &sky_fifo
		  ){
 LOOP_READ_CAL_SKY_STREAM:
  for(int i = 0; i < NBURST_PER_TIME; i++){
#pragma HLS PIPELINE
    cal_pol1_fifo << cal_pol1[i];
    cal_pol2_fifo << cal_pol2[i];
    sky_fifo      << sky[i]; 
  }
}

void read_in(
	     burst_data_type *in_pol1,
	     burst_data_type *in_pol2,
	     fifo_data_type &in_pol1_fifo,
	     fifo_data_type &in_pol2_fifo
	     ){
 LOOP_READ_IN_STREAM_I:
  for(int i = 0; i < NBURST_PER_TIME; i++){
  LOOP_READ_IN_STREAM_J:
    for(int j = 0; j < NTIME_PER_CU; j++){
#pragma HLS PIPELINE
      int loc = j * NBURST_PER_TIME + i;
      in_pol1_fifo << in_pol1[loc];
      in_pol2_fifo << in_pol2[loc];
    }
  }
}

void write_out(
	       fifo_data_type &out_fifo,
	       burst_data_type *out
	       ){
 LOOP_WRITE_OUT_STREAM_I:
  for(int i = 0; i < NBURST_PER_TIME; i++){
  LOOP_WRITE_OUT_STREAM_J:
    for(int j = 0; j < NTIME_PER_CU; j++){
#pragma HLS PIPELINE
      int loc = j * NBURST_PER_TIME + i;
      out_fifo >> out[loc];
    }
  }
}

void write_average(		   
		   fifo_data_type &average_pol1_fifo,
		   fifo_data_type &average_pol2_fifo,
		   burst_data_type *average_pol1,
		   burst_data_type *average_pol2
				   ){  
 LOOP_WRITE_AVERAGE_STREAM:
  for(int i = 0; i < NBURST_PER_TIME; i++){
#pragma HLS PIPELINE II=1
    average_pol1_fifo >> average_pol1[i];
    average_pol2_fifo >> average_pol2[i];
  }
}

void compute_out_average(
			 fifo_data_type &in_pol1_fifo,
			 fifo_data_type &in_pol2_fifo,
			 fifo_data_type &cal_pol1_fifo,
			 fifo_data_type &cal_pol2_fifo,
			 fifo_data_type &sky_fifo,
			 fifo_data_type &out_fifo,
			 fifo_data_type &average_pol1_fifo,
			 fifo_data_type &average_pol2_fifo
			 
			 ){
  burst_data_type in_pol1_burst;
  burst_data_type in_pol2_burst;
  burst_data_type cal_pol1_burst;
  burst_data_type cal_pol2_burst;
  burst_data_type sky_burst;
  burst_data_type out_burst;  
  burst_data_type average_pol1_burst;
  burst_data_type average_pol2_burst;
    
#pragma HLS DATA_PACK variable = in_pol1_burst
#pragma HLS DATA_PACK variable = in_pol2_burst
#pragma HLS DATA_PACK variable = cal_pol1_burst
#pragma HLS DATA_PACK variable = cal_pol2_burst
#pragma HLS DATA_PACK variable = out_burst
#pragma HLS DATA_PACK variable = sky_burst
#pragma HLS DATA_PACK variable = average_pol1_burst
#pragma HLS DATA_PACK variable = average_pol2_burst

  float temp;
 LOOP_COMPUTE_OUT_AVERAGE_STREAM:
  for(int i = 0; i < NBURST_PER_TIME; i++){
    cal_pol1_fifo >> cal_pol1_burst ;
    cal_pol2_fifo >> cal_pol2_burst ;
    sky_fifo      >> sky_burst ;
    
  LOOP_NSAMP_PER_BURST1:
    for(int k = 0; k < NDATA_PER_BURST; k++){
#pragma HLS UNROLL
      average_pol1_burst.data[k]   = 0;
      average_pol2_burst.data[k]   = 0;
    }	  
	  
  LOOP_NTIME_PER_CU:
    for(int j = 0; j < NTIME_PER_CU; j++){
#pragma HLS PIPELINE 
      
      in_pol1_fifo >> in_pol1_burst ;
      in_pol2_fifo >> in_pol2_burst ;
      
    LOOP_NSAMP_PER_BURST2:
      for(int k = 0; k < NSAMP_PER_BURST; k++){
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
      
      out_fifo << out_burst;
    }

    average_pol1_fifo << average_pol1_burst;
    average_pol2_fifo << average_pol2_burst;
  }
}

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

    fifo_data_type in_pol1_fifo;
    fifo_data_type in_pol2_fifo;
    fifo_data_type cal_pol1_fifo;
    fifo_data_type cal_pol2_fifo;
    fifo_data_type sky_fifo;
    fifo_data_type out_fifo;
    fifo_data_type average_pol1_fifo;
    fifo_data_type average_pol2_fifo;

#pragma HLS DATAFLOW
    read_cal_sky(cal_pol1,cal_pol2, sky, cal_pol1_fifo, cal_pol2_fifo, sky_fifo);
    read_in(in_pol1, in_pol2, in_pol1_fifo, in_pol1_fifo);
    compute_out_average(in_pol1_fifo, in_pol2_fifo, cal_pol1_fifo, cal_pol2_fifo, sky_fifo, out_fifo, average_pol1_fifo, average_pol2_fifo);
    write_out(out_fifo, out);
    write_average(average_pol1_fifo, average_pol2_fifo, average_pol1, average_pol2);
  }
}
