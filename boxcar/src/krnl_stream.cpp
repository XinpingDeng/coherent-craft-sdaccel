#include "boxcar.h"

extern "C"{  
  void krnl_stream(
                   burst_real *in,                        
                   int ndm,
                   int ntime,
                   int nburst_per_img,
                   stream_burst &out
                   );
}

void krnl_stream(
                 burst_real *in,                        
                 int ndm,
                 int ntime,
                 int nburst_per_img,
                 stream_burst &out
                 ){  
#pragma HLS INTERFACE axis      port = out
#pragma HLS INTERFACE m_axi     port = in  offset = slave bundle = gmem0

#pragma HLS INTERFACE s_axilite port = in             bundle = control
#pragma HLS INTERFACE s_axilite port = ndm            bundle = control
#pragma HLS INTERFACE s_axilite port = ntime          bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_img bundle = control
#pragma HLS INTERFACE s_axilite port = return         bundle = control

  int i;
  const int max_loop_i = MAX_DM*MAX_TIME*MAX_BURST_PER_IMG;
  stream_burst_core burst;
  
 loop_stream:
  for(i = 0; i < ndm*ntime*nburst_per_img; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_loop_i
#pragma HLS PIPELINE
    burst.data = in[i];
    out.write(burst);
  }  
}
