#include "krnl.h"

extern "C"{
  void krnl_write(                 
                  int nplane,
                  int nburst_per_img,
                  stream_cmplx &in_stream,
                  burst_cmplx *out);
}

void krnl_write(                 
                int nplane,
                int nburst_per_img,
                stream_cmplx &in_stream,
                burst_cmplx *out){
  const int burst_len = BURST_LEN_WRITE;
#pragma HLS INTERFACE m_axi port = out       offset = slave bundle = gmem0 max_write_burst_length = burst_len
#pragma HLS INTERFACE axis  port = in_stream

#pragma HLS INTERFACE s_axilite port = nplane         bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_img bundle = control
#pragma HLS INTERFACE s_axilite port = out            bundle = control
#pragma HLS INTERFACE s_axilite port = return         bundle = control

  const int max_plane = MAX_PLANE;
  const int max_burst_per_img = MAX_BURST_PER_IMG;
  
  int i;
  int j;
  int k;
  int loc;
  burst_cmplx burst;
  stream_cmplx_core stream;

#pragma HLS DATA_PACK variable=out
#pragma HLS DATA_PACK variable=burst
  
  for(i = 0; i < nplane; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_plane
  loop_write:
    
#ifndef __SYNTHESIS__
    fprintf(stdout, "HERE KERNEL_WRITE\t%d\n", i);
#endif
    
    for(j = 0; j < nburst_per_img; j++){
#pragma HLS LOOP_TRIPCOUNT max = max_burst_per_img
#pragma HLS PIPELINE
      stream = in_stream.read();
      for(k = 0; k < NCMPLX_PER_BURST; k++){
        burst.data[k] = stream.data((k+1)*CMPLX_WIDTH-1, k*CMPLX_WIDTH);
      }
      loc = i*nburst_per_img + j;
      out[loc] = burst;
    }
  }
}
