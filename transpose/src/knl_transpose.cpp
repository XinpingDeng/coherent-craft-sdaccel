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
  
  void read2fifo(
                 int nburst_per_uv_out,
                 int ntime_per_cu,
                 int nburst_dm,
                 const burst_uv *in,
                 const burst_coord *coord,
                 fifo_uv &fifo_in);
  
  void transpose(
                 int nburst_per_uv_out,
                 int ntime_per_cu,
                 int nburst_dm,
                 fifo_uv &fifo_in,
                 fifo_uv &fifo_out);

  void fill_tile(
                 fifo_uv &fifo_in,
                 uv_samp_t uv_tile[TILE_WIDTH][TILE_WIDTH]);
  
  void transpose_tile(
                      uv_samp_t uv_tile[TILE_WIDTH][TILE_WIDTH],
                      fifo_uv &fifo_in);
  
  void write_from_fifo(
                       int nburst_per_uv_out,
                       int ntime_per_cu,
                       int nburst_dm,
                       fifo_uv &fifo_out,
                       burst_uv *out);
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
  //const int burst_length      = BURST_LENGTH;
  
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 //max_read_burst_length =burst_length
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 //max_write_burst_length=burst_length

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

  fifo_uv fifo_in;
  fifo_uv fifo_out;
  const int nburst_per_tran = BURST_LENGTH*BURST_LENGTH;
#pragma HLS STREAM variable = fifo_in  depth = nburst_per_tran //256 //32
#pragma HLS STREAM variable = fifo_out depth = nburst_per_tran //256 //32
  
#pragma HLS DATAFLOW
  
  read2fifo(
            nburst_per_uv_out,
            ntime_per_cu,
            nburst_dm,
            in,
            coord,
            fifo_in);
  
  transpose(
            nburst_per_uv_out,
            ntime_per_cu,
            nburst_dm,
            fifo_in,
            fifo_out);

  write_from_fifo(
                  nburst_per_uv_out,
                  ntime_per_cu,
                  nburst_dm,
                  fifo_out,
                  out);
}

void read2fifo(
               int nburst_per_uv_out,
               int ntime_per_cu,
               int nburst_dm,
               const burst_uv *in,
               const burst_coord *coord,
               fifo_uv &fifo_in){
  int i;
  int j;
  int k;
  int m;
  int n;
  int loc_burst0;
  int loc_burst;
  int ntran_dm         = nburst_dm/BURST_LENGTH;
  int ntran_per_uv_out = nburst_per_uv_out/BURST_LENGTH;
  int nsamp_per_burst  = NSAMP_PER_BURST;

  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  const int mtran_per_uv_out = MBURST_PER_UV_OUT/BURST_LENGTH;
  const int mtime_per_cu     = MTIME_PER_CU;  
  const int mtran_dm         = MBURST_DM/BURST_LENGTH;
  
  coord_t2 coord_buffer[MSAMP_PER_UV_OUT];
  
#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor = nsamp_per_burst
  
  // Burst in coord;
 loop_burst_coord:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mburst_per_uv_out 
    for(j = 0; j < NSAMP_PER_BURST; j++){
      coord_buffer[i*NSAMP_PER_BURST+j] = coord[i].data[j];
    }
  }

  for(k = 0; k < ntime_per_cu; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtime_per_cu
    for(i = 0; i < ntran_per_uv_out; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_per_uv_out
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_dm
        
        // Burst in
        for(m = 0; m < TILE_WIDTH; m++){            // For UV
          loc_burst0 = coord_buffer[i*TILE_WIDTH+m]*ntime_per_cu*nburst_dm+
            k*nburst_dm+
            j*BURST_LENGTH;
        loop_read2fifo:
          for(n = 0; n < BURST_LENGTH; n++){        // For DM
#pragma HLS PIPELINE
            loc_burst = loc_burst0 +
              n;
            fifo_in.write(in[loc_burst]);
          }
        }        
      }      
    }
  }
}

void transpose(
               int nburst_per_uv_out,
               int ntime_per_cu,
               int nburst_dm,
               fifo_uv &fifo_in,
               fifo_uv &fifo_out){
  int i;
  int j;
  int k;
  int ntran_dm         = nburst_dm/BURST_LENGTH;
  int ntran_per_uv_out = nburst_per_uv_out/BURST_LENGTH;
  
  const int mtran_per_uv_out = MBURST_PER_UV_OUT/BURST_LENGTH;
  const int mtime_per_cu     = MTIME_PER_CU;  
  const int mtran_dm         = MBURST_DM/BURST_LENGTH;
  const int nsamp_per_burst  = NSAMP_PER_BURST;
  
  uv_samp_t uv_tile[TILE_WIDTH][TILE_WIDTH];
#pragma HLS ARRAY_RESHAPE variable = uv_tile    cyclic factor = nsamp_per_burst dim =1
#pragma HLS ARRAY_RESHAPE variable = uv_tile    cyclic factor = nsamp_per_burst dim =2

  for(k = 0; k < ntime_per_cu; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtime_per_cu
    for(i = 0; i < ntran_per_uv_out; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_per_uv_out
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_dm
#pragma HLS DATAFLOW
        
        fill_tile(fifo_in, uv_tile);
        transpose_tile(uv_tile, fifo_out);
      }      
    }
  }  
}

void fill_tile(
               fifo_uv &fifo_in,
               uv_samp_t uv_tile[TILE_WIDTH][TILE_WIDTH]){
  
  int m;
  int n;
  int n1;
  int loc_tile;
  burst_uv burst;
  
  for(m = 0; m < TILE_WIDTH; m++){              // For UV
  loop_fill_tile:
    for(n = 0; n < BURST_LENGTH; n++){          // For DM
#pragma HLS PIPELINE
      burst = fifo_in.read();
      for(n1 = 0; n1 < NSAMP_PER_BURST; n1++){  // For DM
        loc_tile = n*NSAMP_PER_BURST+n1;
        uv_tile[m][loc_tile] = burst.data[n1];
      }
    }
  }
}

void transpose_tile(
                    uv_samp_t uv_tile[TILE_WIDTH][TILE_WIDTH],
                    fifo_uv &fifo_out){
  int m;
  int n;
  int n1;
  int loc_tile;
  burst_uv burst;
  
  for(m = 0; m < TILE_WIDTH; m++){              // For DM
  loop_transpose_tile:
    for(n = 0; n < BURST_LENGTH; n++){          // For UV
#pragma HLS PIPELINE            
      for(n1 = 0; n1 < NSAMP_PER_BURST; n1++){  // For UV
        loc_tile = n*NSAMP_PER_BURST+n1;
        burst.data[n1] = uv_tile[loc_tile][m];
      }
      fifo_out.write(burst);
    }          
  }
}

void write_from_fifo(
                     int nburst_per_uv_out,
                     int ntime_per_cu,
                     int nburst_dm,
                     fifo_uv &fifo_out,
                     burst_uv *out){
  int i;
  int j;
  int k;
  int m;
  int n;
  int loc_burst;
  int ntran_dm         = nburst_dm/BURST_LENGTH;
  int ntran_per_uv_out = nburst_per_uv_out/BURST_LENGTH;
  
  const int mtran_per_uv_out = MBURST_PER_UV_OUT/BURST_LENGTH;
  const int mtime_per_cu     = MTIME_PER_CU;  
  const int mtran_dm         = MBURST_DM/BURST_LENGTH;
  
  for(k = 0; k < ntime_per_cu; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtime_per_cu
    for(i = 0; i < ntran_per_uv_out; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_per_uv_out
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = mtran_dm
        
        // Burst out
        for(m = 0; m < TILE_WIDTH; m++){            // For DM
        loop_write_from_fifo:
          for(n = 0; n < BURST_LENGTH; n++){        // For UV
#pragma HLS PIPELINE
            loc_burst = (j*TILE_WIDTH+m)*ntime_per_cu*nburst_per_uv_out +
              k*nburst_per_uv_out +
              i*BURST_LENGTH +
              n;
            out[loc_burst] = fifo_out.read();
          }
        }        
      }      
    }
  }
}
