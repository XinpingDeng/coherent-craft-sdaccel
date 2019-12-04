#include "grid_knl.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
void knl_boxcar(
		const burst_t *in,
		burst_t *out1,
		burst_t *out2,
		burst_t *out3,
		int ndm,
		int ntime
		);  
  void boxcar1(
	       const burst_t *in,
	       stream_t &boxcar1_stream,
	       stream_t &in_stream_hold1,
	       stream_t &boxcar1_stream_out,
	       int ndm,
	       int ntime);
  
  void boxcar2(
	       stream_t &boxcar1_stream,
	       stream_t &boxcar1_stream_hold,
	       stream_t &boxcar2_stream,
	       stream_t &boxcar2_stream_hold,
	       stream_t &boxcar2_stream_out,
	       int ndm,
	       int ntime);
  
  void boxcar3(
	       stream_t &boxcar2_stream,
	       stream_t &boxcar2_stream_hold,
	       stream_t &boxcar3_stream_out,
	       int ndm,
	       int ntime);
  
  void write_out(
		 stream_t &boxcar1_stream_out,
		 stream_t &boxcar2_stream_out,
		 stream_t &boxcar3_stream_out,
		 burst_t *out1,
		 burst_t *out2,
		 burst_t *out3,
		 int ndm,
		 int ntime	       
		 );
}

void knl_boxcar(
		const burst_t *in,
		burst_t *out1,
		burst_t *out2,
		burst_t *out3,
		int ndm,
		int ntime
		)
{
#pragma HLS INTERFACE m_axi port = in   offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = out1 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out2 offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = out3 offset = slave bundle = gmem3

#pragma HLS INTERFACE s_axilite port = in     bundle = control
#pragma HLS INTERFACE s_axilite port = out1   bundle = control
#pragma HLS INTERFACE s_axilite port = out2   bundle = control
#pragma HLS INTERFACE s_axilite port = out3   bundle = control
#pragma HLS INTERFACE s_axilite port = ndm    bundle = control
#pragma HLS INTERFACE s_axilite port = ntime  bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out1
#pragma HLS DATA_PACK variable = out2
#pragma HLS DATA_PACK variable = out3
  
  stream_t boxcar1_stream;
  stream_t boxcar2_stream;
  stream_t boxcar3_stream;
  stream_t boxcar4_stream;
  stream_t boxcar5_stream;
  stream_t boxcar6_stream;
  stream_t boxcar7_stream;
  stream_t boxcar8_stream;
  stream_t boxcar9_stream;
  stream_t boxcar10_stream;
  stream_t boxcar11_stream;
  stream_t boxcar12_stream;
  stream_t boxcar13_stream;
  stream_t boxcar14_stream;
  stream_t boxcar15_stream;
  stream_t boxcar16_stream;
  
  stream_t boxcar1_stream_hold;
  stream_t boxcar2_stream_hold;
  stream_t boxcar3_stream_hold;
  stream_t boxcar4_stream_hold;
  stream_t boxcar5_stream_hold;
  stream_t boxcar6_stream_hold;
  stream_t boxcar7_stream_hold;
  stream_t boxcar8_stream_hold;
  stream_t boxcar9_stream_hold;
  stream_t boxcar10_stream_hold;
  stream_t boxcar11_stream_hold;
  stream_t boxcar12_stream_hold;
  stream_t boxcar13_stream_hold;
  stream_t boxcar14_stream_hold;
  stream_t boxcar15_stream_hold;
  stream_t boxcar16_stream_hold;
  
  stream_t boxcar1_stream_out;
  stream_t boxcar2_stream_out;
  stream_t boxcar3_stream_out;
  stream_t boxcar4_stream_out;
  stream_t boxcar5_stream_out;
  stream_t boxcar6_stream_out;
  stream_t boxcar7_stream_out;
  stream_t boxcar8_stream_out;
  stream_t boxcar9_stream_out;
  stream_t boxcar10_stream_out;
  stream_t boxcar11_stream_out;
  stream_t boxcar12_stream_out;
  stream_t boxcar13_stream_out;
  stream_t boxcar14_stream_out;
  stream_t boxcar15_stream_out;
  stream_t boxcar16_stream_out;
  
  // These streams hold multiple images, delay for NBURST_PER_IMG, accumulate happens when new samples come in
  // The size should be NBURST_PER_IMG + 1, but can not use defined variable here
  // Long stream afterwards
#pragma HLS STREAM variable = boxcar1_stream depth = 1025
#pragma HLS STREAM variable = boxcar2_stream depth = 1025
#pragma HLS STREAM variable = boxcar3_stream depth = 1025
#pragma HLS STREAM variable = boxcar4_stream depth = 1025
#pragma HLS STREAM variable = boxcar5_stream depth = 1025
#pragma HLS STREAM variable = boxcar6_stream depth = 1025
#pragma HLS STREAM variable = boxcar7_stream depth = 1025
#pragma HLS STREAM variable = boxcar8_stream depth = 1025
#pragma HLS STREAM variable = boxcar9_stream depth = 1025
#pragma HLS STREAM variable = boxcar10_stream depth = 1025
#pragma HLS STREAM variable = boxcar11_stream depth = 1025
#pragma HLS STREAM variable = boxcar12_stream depth = 1025
#pragma HLS STREAM variable = boxcar13_stream depth = 1025
#pragma HLS STREAM variable = boxcar14_stream depth = 1025
#pragma HLS STREAM variable = boxcar15_stream depth = 1025

//#pragma HLS RESOURCE variable=boxcar1_stream core=XPM_MEMORY uram
//#pragma HLS RESOURCE variable=boxcar2_stream core=XPM_MEMORY uram
//#pragma HLS RESOURCE variable=boxcar3_stream core=XPM_MEMORY uram
  
  // These streams hold one new income sample, the delay between them and the above streams is 1024 (NBURST_PER_IMG)
  // Short stream afterwards
#pragma HLS STREAM variable = boxcar1_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar2_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar3_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar4_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar5_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar6_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar7_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar8_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar9_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar10_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar11_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar12_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar13_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar14_stream_hold depth = 1
#pragma HLS STREAM variable = boxcar15_stream_hold depth = 1

  // These streams hols one outcome sample
#pragma HLS STREAM variable = boxcar1_stream_out depth = 1
#pragma HLS STREAM variable = boxcar2_stream_out depth = 1
#pragma HLS STREAM variable = boxcar3_stream_out depth = 1
#pragma HLS STREAM variable = boxcar4_stream_out depth = 1
#pragma HLS STREAM variable = boxcar5_stream_out depth = 1
#pragma HLS STREAM variable = boxcar6_stream_out depth = 1
#pragma HLS STREAM variable = boxcar7_stream_out depth = 1
#pragma HLS STREAM variable = boxcar8_stream_out depth = 1
#pragma HLS STREAM variable = boxcar9_stream_out depth = 1
#pragma HLS STREAM variable = boxcar10_stream_out depth = 1
#pragma HLS STREAM variable = boxcar11_stream_out depth = 1
#pragma HLS STREAM variable = boxcar12_stream_out depth = 1
#pragma HLS STREAM variable = boxcar13_stream_out depth = 1
#pragma HLS STREAM variable = boxcar14_stream_out depth = 1
#pragma HLS STREAM variable = boxcar15_stream_out depth = 1
#pragma HLS STREAM variable = boxcar16_stream_out depth = 1

#pragma HLS DATAFLOW
  
  boxcar1(in,
	  boxcar1_stream,
	  boxcar1_stream_hold,
	  boxcar1_stream_out,
	  ndm,
	  ntime);
  
  boxcar2(
	  boxcar1_stream,
	  boxcar1_stream_hold,
	  boxcar2_stream,
	  boxcar2_stream_hold,
	  boxcar2_stream_out,
	  ndm,
	  ntime);
  
  boxcar3(
	  boxcar2_stream,
	  boxcar2_stream_hold,
	  boxcar3_stream_out,
	  ndm,
	  ntime);
  
  write_out(
	    boxcar1_stream_out,
	    boxcar2_stream_out,
	    boxcar3_stream_out,
	    out1,
	    out2,
	    out3,
	    ndm,
	    ntime	       
	    );
}

void boxcar1(
	     const burst_t *in,
	     stream_t &boxcar1_stream,
	     stream_t &boxcar1_stream_hold,
	     stream_t &boxcar1_stream_out,
	     int ndm,
	     int ntime){
  int i;
  int j;
  int m;
  int n;
  int loc;
  burst_t in_burst;

 LOOP_BOXCAR1_I:
  for(i = 0; i < ndm; i++){
  LOOP_BOXCAR1_J1:
    // Read in the first image of each DM, get boxcar1 and setup for boxcar2
    for(j = 0; j < NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      loc = i*NBURST_PER_IMG*ntime + j;
      in_burst = in[loc];
      // Boxcar1
      boxcar1_stream_out.write(in_burst);
      // Setup for boxcar2
      // Here only setup the long stream
      boxcar1_stream.write(in_burst);
    }

  LOOP_BOXCAR1_M:
    // Read in the rest images expect the last one, get boxcar1 and setup for boxcar2
    for(m = 1; m < (ntime - 1); m++){
    LOOP_BOXCAR1_N:
      for(n = 0; n < NBURST_PER_IMG; n++){
#pragma HLS PIPELINE
	loc = i*ntime*NBURST_PER_IMG + m*NBURST_PER_IMG + n;
	in_burst = in[loc];
	// Boxcar1
	boxcar1_stream_out.write(in_burst);
	// Setup for boxcar2
	// Here setup both long and short stream
	boxcar1_stream.write(in_burst);
	boxcar1_stream_hold.write(in_burst);
      }
    }
    
  LOOP_BOXCAR1_J2:
    // Read in the last image of each DM, get boxcar1 and setup for boxcar2
    for(j = 0; j < NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      loc = i*ntime*NBURST_PER_IMG + (ntime-1)*NBURST_PER_IMG + n;
      in_burst = in[loc];
      // Boxcar1
      boxcar1_stream_out.write(in_burst);
      // Here only setup the short stream
      // Setup for boxcar2
      boxcar1_stream_hold.write(in_burst);
    }
  }
}

void boxcar2(
	     stream_t &boxcar1_stream,
	     stream_t &boxcar1_stream_hold,
	     stream_t &boxcar2_stream,
	     stream_t &boxcar2_stream_hold,
	     stream_t &boxcar2_stream_out,
	     int ndm,
	     int ntime){
  burst_t burst_previous;
  burst_t burst_current;
  burst_t burst_result;

  int i;
  int j;
  int m;
  int n;
    
 LOOP_BOXCAR2_I:
  for(i = 0; i < ndm; i++){
  LOOP_BOXCAR2_M1:
    // Calculate the first boxcar2 of each DM and setup for boxcar3
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      // Read boxcar1 long and short stream
      burst_previous = boxcar1_stream.read();
      burst_current  = boxcar1_stream_hold.read();

      // Calculate boxcar2
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
      }
      // Sendout boxcar2
      boxcar2_stream_out.write(burst_result);
      // Setup boxcar3
      // Here only setup long stream
      boxcar2_stream.write(burst_current);
    }
    
  LOOP_BOXCAR2_J:
    // Calculate the rest boxcar2 (except the last one) of each DM and setup for boxcar3
    for(j = 2; j < (ntime-1); j++){
    LOOP_BOXCAR2_M2:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	// Read boxcar1 long and short stream
	burst_previous = boxcar1_stream.read();
	burst_current  = boxcar1_stream_hold.read();

	// Calculate boxcar2
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
	}

	// Sendout boxcar2
	boxcar2_stream_out.write(burst_result);
	// Setup boxcar3
	// Here setup both long and short stream
	boxcar2_stream.write(burst_result);
	boxcar2_stream_hold.write(burst_current);
      }
    }
    
  LOOP_BOXCAR2_M3:
    // Calculate the last boxcar2 of each DM and setup for boxcar3
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      // Read in boxcar1 long and short stream
      burst_previous = boxcar1_stream.read();
      burst_current  = boxcar1_stream_hold.read();

      // Calculate boxcar2
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
      }

      // Sendout boxcar2
      boxcar2_stream_out.write(burst_result);
      // Setup boxcar3
      // Here only setup short stream
      boxcar2_stream_hold.write(burst_current);
    }    
  }  
}

void boxcar3(
	     stream_t &boxcar2_stream,
	     stream_t &boxcar2_stream_hold,
	     stream_t &boxcar3_stream_out,
	     int ndm,
	     int ntime){
  burst_t burst_previous;
  burst_t burst_current;
  burst_t burst_result;

  int i;
  int j;
  int m;
  int n;
    
 LOOP_BOXCAR3_I:
  for(i = 0; i < ndm; i++){
  LOOP_BOXCAR3_M1:
    // Calculate the first boxcar3 of each DM
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      // Read boxcar2 long and short stream
      burst_previous = boxcar2_stream.read();
      burst_current  = boxcar2_stream_hold.read();

      // Calculate boxcar3
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
      }
      // Sendout boxcar3
      boxcar3_stream_out.write(burst_result);
    }
    
  LOOP_BOXCAR3_J:
    // Calculate the rest boxcar3 (except the last one) of each DM 
    for(j = 3; j < (ntime-1); j++){
    LOOP_BOXCAR3_M2:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	// Read boxcar2 long and short stream
	burst_previous = boxcar2_stream.read();
	burst_current  = boxcar2_stream_hold.read();

	// Calculate boxcar3
	for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	  burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
	}

	// Sendout boxcar3
	boxcar3_stream_out.write(burst_result);
      }
    }
    
  LOOP_BOXCAR3_M3:
    // Calculate the last boxcar3 of each DM 
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      // Read in boxcar2 long and short stream
      burst_previous = boxcar2_stream.read();
      burst_current  = boxcar2_stream_hold.read();

      // Calculate boxcar3
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	burst_result.data[n] = burst_previous.data[n] + burst_current.data[n];
      }

      // Sendout boxcar3
      boxcar3_stream_out.write(burst_result);
    }    
  }  
}

void write_out(
	       stream_t &boxcar1_stream_out,
	       stream_t &boxcar2_stream_out,
	       stream_t &boxcar3_stream_out,
	       burst_t *out1,
	       burst_t *out2,
	       burst_t *out3,
	       int ndm,
	       int ntime	       
	       ){
  
  int i;
  int j;
  int m;
  int loc;

 LOOP_WRITE_I:
  for(i = 0; i < ndm; i++){
  LOOP_WRITE_M1:
    // Write the first boxcar1
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      loc = i*ntime*NBURST_PER_IMG + m;
      out1[loc] = boxcar1_stream_out.read();
    }

  LOOP_WRITE_M2:
    // Write the first boxcar2
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      loc = i*ntime*NBURST_PER_IMG + NBURST_PER_IMG + m;
      out1[loc] = boxcar1_stream_out.read();
      
      loc = i*ntime*NBURST_PER_IMG + m;
      out2[loc] = boxcar2_stream_out.read();
    }

  LOOP_WRITE_J:
    // Write the rest of boxcar1 and boxcar2
    // Write all boxcar3
    for(j = 2; j < ntime; j++){
    LOOP_WRITE_M3:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	loc = i*ntime*NBURST_PER_IMG + j*NBURST_PER_IMG + m;
	out1[loc] = boxcar1_stream_out.read();

	loc = i*ntime*NBURST_PER_IMG + (j-1)*NBURST_PER_IMG + m;
	out2[loc] = boxcar2_stream_out.read();
	
	loc = i*ntime*NBURST_PER_IMG + (j-2)*NBURST_PER_IMG + m;
	out3[loc] = boxcar3_stream_out.read();	
      }      
    }
  }
}
