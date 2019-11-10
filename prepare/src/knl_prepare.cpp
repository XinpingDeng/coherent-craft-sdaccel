#include "prepare.h"

void read(
	  burst_data_type *in_pol1,
	  burst_data_type *in_pol2,
	  stream_data_type &in_pol1_stream,
	  stream_data_type &in_pol2_stream){

 READ_I:
  for(int i = 0; i < NTRAN_PER_TIME; i++){
  READ_J:
    for(int j = 0; j < NTIME_PER_CU; j++){
    READ_M:
      for(int m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE REWIND
	int loc = j*NTRAN_PER_TIME*MAX_BURST_LENGTH + i*MAX_BURST_LENGTH + m;	  
	in_pol1_stream.write(in_pol1[loc]);
	in_pol2_stream.write(in_pol2[loc]);	
      }
    }
  }
}

void write(
	   stream_data_type &out_stream,
	   burst_data_type *out){

 WRITE_I:
  for(int i = 0; i < NTRAN_PER_TIME; i++){
  WRITE_J:
    for(int j = 0; j < NTIME_PER_CU; j++){
    WRITE_M:
    for(int m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE REWIND
	int loc = j*NTRAN_PER_TIME*MAX_BURST_LENGTH + i*MAX_BURST_LENGTH + m;	  
	out[loc] = out_stream.read();
      }
    }
  }  
}

void process(
	     stream_data_type &in_pol1_stream,
	     stream_data_type &in_pol2_stream,
	     burst_data_type *cal_pol1,
	     burst_data_type *cal_pol2,
	     burst_data_type *sky,
	     stream_data_type &out_stream,
	     burst_data_type *average_pol1,
	     burst_data_type *average_pol2){

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

  core_data_type average_pol1_temp1;
  core_data_type average_pol1_temp2;
  core_data_type average_pol2_temp1;
  core_data_type average_pol2_temp2;
  
 PROCESS_I:
  for(i = 0; i < NTRAN_PER_TIME; i++){
  READ_CAL_SKY_RESET_AVERAGE:
    for(j = 0; j < MAX_BURST_LENGTH; j++){
      loc = i*MAX_BURST_LENGTH + j;
      cal_pol1_burst[j] = cal_pol1[loc];
      cal_pol2_burst[j] = cal_pol2[loc];
      sky_burst[j]      = sky[loc];
      
    RESET_AVERAGE:
      for(m = 0; m < NDATA_PER_BURST; m++){
	average_pol1_burst[j].data[m] = 0;
	average_pol2_burst[j].data[m] = 0;	
      }
    }
    
  PROCESS_J:
    for(j = 0; j < NTIME_PER_CU; j++){
    PROCESS_M:
      for(m = 0; m < MAX_BURST_LENGTH; m++){
#pragma HLS PIPELINE REWIND
#pragma HLS LOOP_FLATTEN
	in_pol1_burst = in_pol1_stream.read();
	in_pol2_burst = in_pol2_stream.read();
      PROCESS_AVERAGE_N:
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  average_pol1_burst[m].data[2*n]   = average_pol1_burst[m].data[2*n]   + in_pol1_burst.data[2*n];
	  average_pol1_burst[m].data[2*n+1] = average_pol1_burst[m].data[2*n+1] + in_pol1_burst.data[2*n+1];
	  average_pol2_burst[m].data[2*n]   = average_pol2_burst[m].data[2*n]   + in_pol2_burst.data[2*n];
	  average_pol2_burst[m].data[2*n+1] = average_pol2_burst[m].data[2*n+1] + in_pol2_burst.data[2*n+1];
	}

      PROCESS_OUT_N:
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  out_burst.data[2*n] = in_pol1_burst.data[2*n]*cal_pol1_burst[m].data[2*n] - in_pol1_burst.data[2*n+1]*cal_pol1_burst[m].data[2*n+1] + 
	    in_pol2_burst.data[2*n]*cal_pol2_burst[m].data[2*n] - in_pol2_burst.data[2*n+1]*cal_pol2_burst[m].data[2*n+1] - 
	    sky_burst[m].data[2*n];
	  
	  out_burst.data[2*n+1] = in_pol1_burst.data[2*n]*cal_pol1_burst[m].data[2*n+1] + in_pol1_burst.data[2*n+1]*cal_pol1_burst[m].data[2*n] + 
	    in_pol2_burst.data[2*n]*cal_pol2_burst[m].data[2*n+1] + in_pol2_burst.data[2*n+1]*cal_pol2_burst[m].data[2*n] - 
	    sky_burst[m].data[2*n+1];
	}
	out_stream.write(out_burst);
      }
    }
    
  WRITE_AVERAGE:
    for(j = 0; j < MAX_BURST_LENGTH; j++){
      loc = i*MAX_BURST_LENGTH + j;
      average_pol1[loc] = average_pol1_burst[j];
      average_pol2[loc] = average_pol2_burst[j];
    }
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
#pragma HLS INTERFACE m_axi port = in_pol1 offset = slave bundle = gmem0 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = in_pol2 offset = slave bundle = gmem1 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = cal_pol1 offset = slave bundle = gmem2 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = cal_pol2 offset = slave bundle = gmem3 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = sky offset = slave bundle = gmem4 max_read_burst_length=64
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem5 max_write_burst_length=64
#pragma HLS INTERFACE m_axi port = average_pol1 offset = slave bundle = gmem6 max_write_burst_length=64
#pragma HLS INTERFACE m_axi port = average_pol2 offset = slave bundle = gmem7 max_write_burst_length=64

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

    stream_data_type in_pol1_stream;
    stream_data_type in_pol2_stream;
    stream_data_type out_stream;

#pragma HLS STREAM variable=in_pol1_stream
#pragma HLS STREAM variable=in_pol2_stream
#pragma HLS STREAM variable=out_stream
    
#pragma HLS DATAFLOW
    read(in_pol1,
	 in_pol2,
	 in_pol1_stream,
	 in_pol2_stream);
    
    process(in_pol1_stream,
	    in_pol2_stream,
	    cal_pol1,
	    cal_pol2,
	    sky,
	    out_stream,
	    average_pol1,
	    average_pol2);
    
    write(out_stream,
	  out);
  }
}
