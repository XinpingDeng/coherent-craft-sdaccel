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
  const int burst_length      = BURST_LENGTH;
  const int nsamp_per_burst   = NSAMP_PER_BURST;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  const int mtime_per_cu      = MTIME_PER_CU;  
  const int mtran_dm          = MBURST_DM/BURST_LENGTH;
  const int ntran_dm          = nburst_dm/BURST_LENGTH;         // To start with a good number here
  const int mtran_per_uv_out  = MBURST_PER_UV_OUT/BURST_LENGTH;
  const int ntran_per_uv_out  = nburst_per_uv_out/BURST_LENGTH; // To start with a good number here
  
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 max_read_burst_length =burst_length
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_write_burst_length=burst_length

#pragma HLS INTERFACE s_axilite port = in     bundle = control
#pragma HLS INTERFACE s_axilite port = coord  bundle = control
#pragma HLS INTERFACE s_axilite port = out    bundle = control

#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
#pragma HLS INTERFACE s_axilite port = ntime_per_cu      bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_dm         bundle = control
#pragma HLS INTERFACE s_axilite port = return            bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = coord

  int i;
  int j;
  int m;
  int n;
  int m1;
  int n1;
  int k;
  int loc_dm;
  int loc_uv;
  int loc_burst;
  burst_uv burst;
  
  coord_t2 coord_buffer[MSAMP_PER_UV_OUT];
  uv_samp_t uv_buffer[BURST_LENGTH*NSAMP_PER_BURST][BURST_LENGTH*NSAMP_PER_BURST];
  
#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor = nsamp_per_burst
#pragma HLS ARRAY_RESHAPE variable = uv_buffer    cyclic factor = nsamp_per_burst dim =1
#pragma HLS ARRAY_RESHAPE variable = uv_buffer    cyclic factor = nsamp_per_burst dim =2
  
  // Burst in coord;
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mburst_per_uv_out 
    for(j = 0; j < NSAMP_PER_BURST; j++){
      coord_buffer[i*NSAMP_PER_BURST+j] = coord[i].data[j];
    }
  }

  for(k = 0; k < ntime_per_cu; k++){
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtime_per_cu
    for(i = 0; i < ntran_per_uv_out; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_per_uv_out
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_dm

        // Burst in
        for(m = 0; m < BURST_LENGTH; m++){            // For UV
          for(m1 = 0; m1 < NSAMP_PER_BURST; m1++){    // For UV
            loc_uv = m*NSAMP_PER_BURST+m1;
            for(n = 0; n < BURST_LENGTH; n++){        // For DM
#pragma HLS PIPELINE
              loc_burst = coord_buffer[(i*BURST_LENGTH+m)*NSAMP_PER_BURST+m1]*ntime_per_cu*nburst_dm+
                k*nburst_dm+
                j*BURST_LENGTH +
                n;
              for(n1 = 0; n1 < NSAMP_PER_BURST; n1++){// For DM
                loc_dm = n*NSAMP_PER_BURST+n1;
                uv_buffer[loc_uv][loc_dm] = in[loc_burst].data[n1];
              }
            }
          }
        }

        // Burst out, transpose at the same time
        for(m = 0; m < BURST_LENGTH; m++){            // For DM
          for(m1 = 0; m1 < NSAMP_PER_BURST; m1++){    // For DM
            loc_dm = m*NSAMP_PER_BURST+m1;
            for(n = 0; n < BURST_LENGTH; n++){        // For UV
#pragma HLS PIPELINE              
              loc_burst = ((j*BURST_LENGTH+m)*NSAMP_PER_BURST+m1)*ntime_per_cu*nburst_per_uv_out +
                k*nburst_per_uv_out +
                i*BURST_LENGTH +
                n;
              for(n1 = 0; n1 < NSAMP_PER_BURST; n1++){// For UV
                loc_uv = n*NSAMP_PER_BURST+n1;
                burst.data[n1] = uv_buffer[loc_uv][loc_dm];
              }
              out[loc_burst] = burst;
            }
          }
        }
      }      
    }
  }
}
