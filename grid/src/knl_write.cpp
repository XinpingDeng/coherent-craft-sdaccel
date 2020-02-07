#include "grid.h"

extern "C"{
  void knl_write(                 
                 int nuv_per_cu,
                 int nburst_per_uv_out,
                 stream_uv &out_stream,
                 burst_uv *out);
}

void knl_write(                 
               int nuv_per_cu,
               int nburst_per_uv_out,
               stream_uv &out_stream,
               burst_uv *out){

#pragma HLS INTERFACE m_axi port = out      offset = slave bundle = gmem2
#pragma HLS INTERFACE axis  port = out_stream

#pragma HLS INTERFACE s_axilite port = nuv_per_cu        bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
#pragma HLS INTERFACE s_axilite port = out               bundle = control
#pragma HLS INTERFACE s_axilite port = return            bundle = control

  const int muv = MUV;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  
  int i;
  int j;
  int k;
  int loc;
  burst_uv burst;
  stream_t stream;

#pragma HLS DATA_PACK variable=out
#pragma HLS DATA_PACK variable=burst
  
  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max = muv
  loop_write:
    
#ifdef __SYNTHESIS__
    fprintf(stdout, "HERE\n");
#endif
    
    for(j = 0; j < nburst_per_uv_out; j++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
#pragma HLS PIPELINE
      stream = out_stream.read();
      for(k = 0; k < NSAMP_PER_BURST; k++){
        burst.data[k] = stream.data(2*(k+1)*DATA_WIDTH-1, 2*k*DATA_WIDTH);
      }
      loc = i*nburst_per_uv_out + j;
      out[loc] = burst;
    }
  }
}
