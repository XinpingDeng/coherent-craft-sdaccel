#include "transpose.h"

// Order is assumed to be UT-TIME-DM
extern "C" {  
  void knl_transpose(
                     const burst_uv *in,
                     const burst_coord *coord,
                     burst_uv *out,
                     int nburst_per_uv_out,
                     int ntime_per_cu,
                     int nburst_dm
                     );
}

void knl_transpose(
                   const burst_uv *in,
                   const burst_coord *coord,
                   burst_uv *out,
                   int nburst_per_uv_out,
                   int ntime_per_cu,
                   int nburst_dm
                   )
{
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_write_burst_length=64

#pragma HLS INTERFACE s_axilite port = in     bundle = control
#pragma HLS INTERFACE s_axilite port = coord  bundle = control
#pragma HLS INTERFACE s_axilite port = out    bundle = control

#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
#pragma HLS INTERFACE s_axilite port = ntime_per_cu bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_dm bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = coord

  int i;
  int j;
  int m;
  int n;
  int k;
  burst_uv in_burst[NSAMP_PER_BURST];
  burst_uv out_burst[NSAMP_PER_BURST];
  int loc_inburst;
  int loc_outburst;
  int loc_in;
  int loc_out;
  int loc_coord;
  coord_t2 start[MSAMP_PER_UV_OUT];
  
  const int nsamp_per_burst   = NSAMP_PER_BURST;
  const int mtime_per_cu      = MTIME_PER_CU;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  const int mburst_dm         = MBURST_DM;
#pragma HLS ARRAY_RESHAPE variable=start cyclic factor = nsamp_per_burst
  
 LOOP_COORD:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_out
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc_coord = i*NSAMP_PER_BURST+j;
      start[loc_coord] = coord[i].data[j];
    }    
  }
  
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_out
    for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT max=mtime_per_cu
    LOOP_TRANSPOSE:
      for(m = 0; m < nburst_dm; m++){
#pragma HLS PIPELINE II=nsamp_per_burst
#pragma HLS LOOP_TRIPCOUNT max=mburst_dm
        // Read in
        for(n = 0; n < NSAMP_PER_BURST; n++){
          loc_inburst = start[i*NSAMP_PER_BURST+n]*ntime_per_cu*nburst_dm +
            j*nburst_dm+
            m;
          in_burst[n] = in[loc_inburst];
        }

        // Transpose
        for(n = 0; n < NSAMP_PER_BURST; n++){
          for(k = 0; k < NSAMP_PER_BURST; k++){
            out_burst[n].data[2*k]   = in_burst[k].data[2*n];
            out_burst[n].data[2*k+1] = in_burst[k].data[2*n+1];
          }
        }

        // Write out
        for(n = 0; n < NSAMP_PER_BURST; n++){
          loc_outburst = (m*NSAMP_PER_BURST+n)*ntime_per_cu*nburst_per_uv_out +
            j*nburst_per_uv_out+
            i;            
          out[loc_outburst] = out_burst[n];
        }
      }
    }
  }
}

