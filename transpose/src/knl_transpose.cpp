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
  const int nsamp_per_burst   = NSAMP_PER_BURST;
  const int ndata_per_burst   = NDATA_PER_BURST;
  const int mtime_per_cu      = MTIME_PER_CU;
  const int mtran_per_uv_out  = MBURST_PER_UV_OUT/BURST_LENGTH;
  const int mtran_dm          = MBURST_DM/BURST_LENGTH;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;

#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 max_read_burst_length=nsamp_per_burst
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_write_burst_length=nsamp_per_burst

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
  int m1;
  int n1;
  int k;
  burst_uv out_burst;
  uv_t buffer[NDATA_PER_BURST*BURST_LENGTH][NDATA_PER_BURST*BURST_LENGTH];
  int loc_burst;
  int loc_uv;
  int loc_dm;
  int loc_coord;
  coord_t2 start[MSAMP_PER_UV_OUT];
    
#pragma HLS ARRAY_RESHAPE variable=buffer cyclic factor = nsamp_per_burst dim = 1
#pragma HLS ARRAY_RESHAPE variable=buffer cyclic factor = ndata_per_burst dim = 2
#pragma HLS ARRAY_RESHAPE variable=start  cyclic factor = nsamp_per_burst

  //FILE *fp = fopen("/data/FRIGG_2/Workspace/coherent-craft-sdaccel/transpose/src/knl_error.txt", "w");
  
 LOOP_COORD:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_out
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc_coord = i*NSAMP_PER_BURST+j;
      start[loc_coord] = coord[i].data[j];
    }    
  }
  
  for(k = 0; k < ntime_per_cu; k++){
#pragma HLS LOOP_TRIPCOUNT max=mtime_per_cu
    for(i = 0; i < nburst_per_uv_out/(2*BURST_LENGTH); i++){
#pragma HLS LOOP_TRIPCOUNT max=mtran_per_uv_out
      for(j = 0; j < nburst_dm/BURST_LENGTH; j++){
#pragma HLS LOOP_TRIPCOUNT max=mtran_dm
        
        // Buffer data for transpose later
        for(m1 = 0; m1 < BURST_LENGTH; m1++){     // For UV
          for(m = 0; m < NDATA_PER_BURST; m++){   // For UV
            loc_uv = m1*NDATA_PER_BURST+m;
          LOOP_IN:
            for(n1 = 0; n1 < BURST_LENGTH; n1++){ // For DM
#pragma HLS PIPELINE
              loc_burst = start[(i*BURST_LENGTH+m1)*NDATA_PER_BURST+m]*ntime_per_cu*nburst_dm +
                k*nburst_dm+
                j*BURST_LENGTH + n1;
              
              for(n = 0; n < NDATA_PER_BURST; n++){
                loc_dm = n1*NDATA_PER_BURST+n;
                buffer[loc_uv][loc_dm] = in[loc_burst].data[n];
                //fprintf(fp, "%f\n", buffer[loc_uv][loc_dm].to_float());
              }
            }
          }
        }
        
        // Write out
        for(m1 = 0; m1 < BURST_LENGTH; m1++){     // For DM
          for(m = 0; m < NDATA_PER_BURST; m++){   // For DM
            loc_dm = m1*NDATA_PER_BURST+m;
          LOOP_OUT:
            for(n1 = 0; n1 < BURST_LENGTH; n1++){ // For UV
#pragma HLS PIPELINE
              loc_burst = (j*NDATA_PER_BURST+m1)*ntime_per_cu*nburst_per_uv_out +
                k*nburst_per_uv_out+
                i*BURST_LENGTH+n1;
              
              for(n = 0; n < NDATA_PER_BURST; n++){
                loc_uv = n1*NDATA_PER_BURST+n;
                out_burst.data[n]   = buffer[loc_uv][loc_dm];
              }
              out[loc_burst] = out_burst;
            }
          }
        }
      }
    }
  }

  //fclose(fp);
}
