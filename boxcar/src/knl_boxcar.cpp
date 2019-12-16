#include "boxcar.h"

// Order is assumed to be DM-TIME-IMAGE
extern "C" {  
  void knl_boxcar(
		  const burst_t *in,
		  burst_t *out1,
		  burst_t *out2,
		  burst_t *out3,
		  burst_t *out4,
		  burst_t *out5,
		  burst_t *out6,
		  burst_t *out7,
		  burst_t *out8,
		  burst_t *out9,
		  burst_t *out10,
		  burst_t *out11,
		  burst_t *out12,
		  burst_t *out13,
		  burst_t *out14,
		  burst_t *out15,
		  burst_t *out16,
		  int ndm,
		  int ntime
		  );  
  void first_boxcar(
		    const burst_t *in,
		    stream_t &current,
		    stream_t &current_hold,
		    burst_t *first_out,
		    int ndm,
		    int ntime);
  
  void mid_boxcar(
		  stream_t &previous,
		  stream_t &previous_hold,
		  stream_t &current,
		  stream_t &current_hold,
		  burst_t *mid_out,
		  int ndm,
		  int ntime,
		  int boxcar);
  
  void last_boxcar(
		   stream_t &previous,
		   stream_t &previous_hold,
		   burst_t *last_out,
		   int ndm,
		   int ntime);
}

void knl_boxcar(
		const burst_t *in,
		burst_t *out1,
		burst_t *out2,
		burst_t *out3,
		burst_t *out4,
		burst_t *out5,
		burst_t *out6,
		burst_t *out7,
		burst_t *out8,
		burst_t *out9,
		burst_t *out10,
		burst_t *out11,
		burst_t *out12,
		burst_t *out13,
		burst_t *out14,
		burst_t *out15,
		burst_t *out16,
		int ndm,
		int ntime
		)
{
  const int long_stream_depth = NBURST_PER_IMG + 1;
  const int max_burst_length = 64;
  
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0  max_read_burst_length =max_burst_length
#pragma HLS INTERFACE m_axi port = out1  offset = slave bundle = gmem1  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out2  offset = slave bundle = gmem2  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out3  offset = slave bundle = gmem3  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out4  offset = slave bundle = gmem4  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out5  offset = slave bundle = gmem5  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out6  offset = slave bundle = gmem6  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out7  offset = slave bundle = gmem7  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out8  offset = slave bundle = gmem8  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out9  offset = slave bundle = gmem9  max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out10 offset = slave bundle = gmem10 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out11 offset = slave bundle = gmem11 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out12 offset = slave bundle = gmem12 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out13 offset = slave bundle = gmem13 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out14 offset = slave bundle = gmem14 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out15 offset = slave bundle = gmem15 max_write_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out16 offset = slave bundle = gmem16 max_write_burst_length=max_burst_length

#pragma HLS INTERFACE s_axilite port = in    bundle = control
#pragma HLS INTERFACE s_axilite port = out1  bundle = control
#pragma HLS INTERFACE s_axilite port = out2  bundle = control
#pragma HLS INTERFACE s_axilite port = out3  bundle = control
#pragma HLS INTERFACE s_axilite port = out4  bundle = control
#pragma HLS INTERFACE s_axilite port = out5  bundle = control
#pragma HLS INTERFACE s_axilite port = out6  bundle = control
#pragma HLS INTERFACE s_axilite port = out7  bundle = control
#pragma HLS INTERFACE s_axilite port = out8  bundle = control
#pragma HLS INTERFACE s_axilite port = out9  bundle = control
#pragma HLS INTERFACE s_axilite port = out10 bundle = control
#pragma HLS INTERFACE s_axilite port = out11 bundle = control
#pragma HLS INTERFACE s_axilite port = out12 bundle = control
#pragma HLS INTERFACE s_axilite port = out13 bundle = control
#pragma HLS INTERFACE s_axilite port = out14 bundle = control
#pragma HLS INTERFACE s_axilite port = out15 bundle = control
#pragma HLS INTERFACE s_axilite port = out16 bundle = control
#pragma HLS INTERFACE s_axilite port = ndm   bundle = control
#pragma HLS INTERFACE s_axilite port = ntime bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out1
#pragma HLS DATA_PACK variable = out2
#pragma HLS DATA_PACK variable = out3
#pragma HLS DATA_PACK variable = out4
#pragma HLS DATA_PACK variable = out5
#pragma HLS DATA_PACK variable = out6
#pragma HLS DATA_PACK variable = out7
#pragma HLS DATA_PACK variable = out8
#pragma HLS DATA_PACK variable = out9
#pragma HLS DATA_PACK variable = out10
#pragma HLS DATA_PACK variable = out11
#pragma HLS DATA_PACK variable = out12
#pragma HLS DATA_PACK variable = out13
#pragma HLS DATA_PACK variable = out14
#pragma HLS DATA_PACK variable = out15
#pragma HLS DATA_PACK variable = out16
  
  // These streams hold multiple images, delay for NBURST_PER_IMG, accumulate happens when new samples come in
  // The size should be NBURST_PER_IMG, but can not use defined variable here
  // Long stream afterwards
  static stream_t boxcar_stream[NBOXCAR-1];
  // These streams hold one new income sample, the delay between them and the above streams is 1024 (NBURST_PER_IMG)
  // Short stream afterwards
  static stream_t boxcar_stream_hold[NBOXCAR-1];
  
  for(int i = 0; i < NBOXCAR-1; i++){
#pragma HLS STREAM variable = boxcar_stream[i] depth = long_stream_depth
#pragma HLS STREAM variable = boxcar_stream_hold[i] 
  }

#pragma HLS DATAFLOW
  // First boxcar
  first_boxcar(in,
	       boxcar_stream[0],
	       boxcar_stream_hold[0],
	       out1,
	       ndm,
	       ntime);
  
  // Boxcar2
  mid_boxcar(
	     boxcar_stream[0],
	     boxcar_stream_hold[0],
	     boxcar_stream[1],
	     boxcar_stream_hold[1],
	     out2,
	     ndm,
	     ntime,
	     2);
  
  // Boxcar3
  mid_boxcar(
	     boxcar_stream[1],
	     boxcar_stream_hold[1],
	     boxcar_stream[2],
	     boxcar_stream_hold[2],
	     out3,
	     ndm,
	     ntime,
	     3);
  
  // Boxcar4
  mid_boxcar(
	     boxcar_stream[2],
	     boxcar_stream_hold[2],
	     boxcar_stream[3],
	     boxcar_stream_hold[3],
	     out4,
	     ndm,
	     ntime,
	     4);
    
  // Boxcar5
  mid_boxcar(
	     boxcar_stream[3],
	     boxcar_stream_hold[3],
	     boxcar_stream[4],
	     boxcar_stream_hold[4],
	     out5,
	     ndm,
	     ntime,
	     5);
  
  // Boxcar6
  mid_boxcar(
	     boxcar_stream[4],
	     boxcar_stream_hold[4],
	     boxcar_stream[5],
	     boxcar_stream_hold[5],
	     out6,
	     ndm,
	     ntime,
	     6);
  
  // Boxcar7
  mid_boxcar(
	     boxcar_stream[5],
	     boxcar_stream_hold[5],
	     boxcar_stream[6],
	     boxcar_stream_hold[6],
	     out7,
	     ndm,
	     ntime,
	     7);
  
  // Boxcar8
  mid_boxcar(
	     boxcar_stream[6],
	     boxcar_stream_hold[6],
	     boxcar_stream[7],
	     boxcar_stream_hold[7],
	     out8,
	     ndm,
	     ntime,
	     8);
  
  // Boxcar9
  mid_boxcar(
	     boxcar_stream[7],
	     boxcar_stream_hold[7],
	     boxcar_stream[8],
	     boxcar_stream_hold[8],
	     out9,
	     ndm,
	     ntime,
	     9);
  
  // Boxcar10
  mid_boxcar(
	     boxcar_stream[8],
	     boxcar_stream_hold[8],
	     boxcar_stream[9],
	     boxcar_stream_hold[9],
	     out10,
	     ndm,
	     ntime,
	     10);
  
  // Boxcar11
  mid_boxcar(
	     boxcar_stream[9],
	     boxcar_stream_hold[9],
	     boxcar_stream[10],
	     boxcar_stream_hold[10],
	     out11,
	     ndm,
	     ntime,
	     11);
  
  // Boxcar12
  mid_boxcar(
	     boxcar_stream[10],
	     boxcar_stream_hold[10],
	     boxcar_stream[11],
	     boxcar_stream_hold[11],
	     out12,
	     ndm,
	     ntime,
	     12);
  
  // Boxcar13
  mid_boxcar(
	     boxcar_stream[11],
	     boxcar_stream_hold[11],
	     boxcar_stream[12],
	     boxcar_stream_hold[12],
	     out13,
	     ndm,
	     ntime,
	     13);
  
  // Boxcar14
  mid_boxcar(
	     boxcar_stream[12],
	     boxcar_stream_hold[12],
	     boxcar_stream[13],
	     boxcar_stream_hold[13],
	     out14,
	     ndm,
	     ntime,
	     14);
  
  // Boxcar15
  mid_boxcar(
	     boxcar_stream[13],
	     boxcar_stream_hold[13],
	     boxcar_stream[14],
	     boxcar_stream_hold[14],
	     out15,
	     ndm,
	     ntime,
	     15);
  
  // Last boxcar
  last_boxcar(
	      boxcar_stream[NBOXCAR-2],
	      boxcar_stream_hold[NBOXCAR-2],
	      out16,
	      ndm,
	      ntime);
}

void first_boxcar(
		  const burst_t *in,
		  stream_t &current,
		  stream_t &current_hold,
		  burst_t *first_out,
		  int ndm,
		  int ntime){
  int i;
  int j;
  int m;
  int n;
  int loc;
  burst_t in_burst;

 LOOP_FIRST_BOXCAR_I:
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024
  LOOP_FIRST_BOXCAR_J:
    // Read in the rest images expect the last one, get boxcar1 and setup for boxcar2
    for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    LOOP_FIRST_BOXCAR_M:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	loc = i*ntime*NBURST_PER_IMG + j*NBURST_PER_IMG + m;
	in_burst = in[loc];
	// Boxcar1
	first_out[loc] = in_burst;
	// Setup for boxcar2
	// Here setup both long and short stream
	if(j<(ntime-1)){
	  current.write(in_burst);
	}
	if(j>0){
	  current_hold.write(in_burst);
	}
      }
    }
  }
}

void mid_boxcar(
		stream_t &previous,
		stream_t &previous_hold,
		stream_t &current,
		stream_t &current_hold,
		burst_t *mid_out,
		int ndm,
		int ntime,
		int boxcar){
  burst_t burst_previous;
  burst_t burst_current;
  burst_t burst_result;
  
  int i;
  int j;
  int m;
  int n;
  int loc;
    
 LOOP_BOXCAR_MID_I:
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024
  LOOP_BOXCAR_MID_J:
    // Calculate the current boxcar of each DM and setup for next boxcar
    for(j = 0; j < (ntime+1-boxcar); j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    LOOP_BOXCAR_MID_M:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	// Read boxcar1 long and short stream
	burst_previous = previous.read();
	burst_current  = previous_hold.read();

	// Calculate current boxcar
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
	}

	// Sendout current boxcar
	loc = i*(ntime+1-boxcar)*NBURST_PER_IMG + j*NBURST_PER_IMG + m;
	mid_out[loc] = burst_result;
	// Setup next boxcar
	// Here setup both long and short stream
	if(j<(ntime-boxcar)){
	  current.write(burst_result);
	}
	if(j>0){
	  current_hold.write(burst_current);
	}
      }
    }  
  }  
}

void last_boxcar(
		 stream_t &previous,
		 stream_t &previous_hold,
		 burst_t *last_out,
		 int ndm,
		 int ntime){
  burst_t burst_previous;
  burst_t burst_current;
  burst_t burst_result;
  
  int i;
  int j;
  int m;
  int n;
  int loc;
    
 LOOP_LAST_BOXCAR_I:
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024
  LOOP_LAST_BOXCAR_J:
    // Calculate the boxcar3 of each DM 
    for(j = 0; j < (ntime+1-NBOXCAR); j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    LOOP_LAST_BOXCAR_M:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	// Read boxcar2 long and short stream
	burst_previous = previous.read();
	burst_current  = previous_hold.read();

	// Calculate boxcar3
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
	}

	// Sendout boxcar3
	loc = i*(ntime+1-NBOXCAR)*NBURST_PER_IMG + j*NBURST_PER_IMG + m;
	last_out[loc] = burst_result;
      }
    }
  }  
}
