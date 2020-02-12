#include "krnl.h"

extern "C"{
  void krnl_fft(                 
                int nplane,
                int nburst_per_img,
                stream_cmplx &in_stream,
                stream_cmplx &out_stream);
}

void krnl_fft(                 
              int nplane,
              int nburst_per_img,
              stream_cmplx &in_stream,
              stream_cmplx &out_stream){
#pragma HLS INTERFACE axis port = in_stream
#pragma HLS INTERFACE axis port = out_stream

#pragma HLS INTERFACE s_axilite port = nplane         bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_img bundle = control
#pragma HLS INTERFACE s_axilite port = return         bundle = control

  const int max_plane = MAX_PLANE;
  const int max_burst_per_img = MAX_BURST_PER_IMG;
  
  int i;
  int j;
  
  for(i = 0; i < nplane; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_plane
  loop_write:
    
#ifndef __SYNTHESIS__
    fprintf(stdout, "HERE KERNEL_WRITE\t%d\n", i);
#endif
    
    for(j = 0; j < nburst_per_img; j++){
#pragma HLS LOOP_TRIPCOUNT max = max_burst_per_img
#pragma HLS PIPELINE
      out_stream.write(in_stream.read());      
    }
  }
}
