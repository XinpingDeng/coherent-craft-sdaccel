#include "grid_knl.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_boxcar(
		  const burst_img *in,
		  burst_img *out,
		  int ndm,
		  int ntime
		  );
}

void knl_boxcar(
		const burst_img *in,
		burst_img *out,
		int ndm,
		int ntime
		)
{
#pragma HLS INTERFACE m_axi port = in  offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem1

#pragma HLS INTERFACE s_axilite port = in    bundle = control
#pragma HLS INTERFACE s_axilite port = out   bundle = control
#pragma HLS INTERFACE s_axilite port = ndm   bundle = control
#pragma HLS INTERFACE s_axilite port = ntime bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out
  
  int i;
  int j;
  int m;
  int n;
  int loc_in;

  burst_img in_burst0;
  burst_img in_burst1;
  stream_img in_stream[NBURST_PER_IMG];

#pragma HLS DATA_PACK variable = in_burst0
#pragma HLS DATA_PACK variable = in_burst1
  
#pragma HLS STREAM variable = in_stream
  
  for(i = 0; i < ndm; i++){
  LOOP_BURST_IN_FIRST_TIME:
    for(j = 0; j < NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      // Fill the stream with the first image of each DM
      loc_in = i*ntime*NBURST_PER_IMG + j;
      in_stream.write(in[loc_in]);
    }
    
    for(m = 1; m < ntime; m++){
      for(n = 0; n < NBURST_PER_IMG; n++){
#pragma HLS PIPELINE
	loc_in    = i*ntime*NBURST_PER_IMG + m*NBURST_PER_IMG + n;

	// in_burst0 and in_burst1 has distance of NBURST_PER_IMG
	in_burst1 = in[loc_in];
	in_stream.read(in_burst0);
	
	// Do magic between in_burst1 and in_burst0
	
	in_stream.write(in_burst1);
      }
    }    
  }
}
