#include "krnl.h"

// Order is assumed to be UT-TIME-DM
extern "C" {  
  void krnl_tran(
                 const burst_cmplx *in,
                 burst_cmplx *out,
                 int nburst_per_uv,
                 int ntime,
                 int nburst_dm_per_cu
                 );
  
  void read2fifo(
                 int nburst_per_uv,
                 int ntime,
                 int nburst_dm_per_cu,
                 const burst_cmplx *in,
                 fifo_cmplx &in_fifo);
  
  void transpose(
                 int nburst_per_uv,
                 int ntime,
                 int nburst_dm_per_cu,
                 fifo_cmplx &in_fifo,
                 fifo_cmplx &out_fifo);

  void fill_tile(
                 fifo_cmplx &in_fifo,
                 cmplx_t cmplx_tile[TILE_WIDTH_TRAN][TILE_WIDTH_TRAN]);
  
  void transpose_tile(
                      cmplx_t cmplx_tile[TILE_WIDTH_TRAN][TILE_WIDTH_TRAN],
                      fifo_cmplx &in_fifo);
  
  void write_from_fifo(
                       int nburst_per_uv,
                       int ntime,
                       int nburst_dm_per_cu,
                       fifo_cmplx &out_fifo,
                       burst_cmplx *out);
}

void krnl_tran(
               const burst_cmplx *in,
               burst_cmplx *out,
               int nburst_per_uv,
               int ntime,
               int nburst_dm_per_cu
               )
{
  const int burst_len = BURST_LEN_TRAN;
  
#pragma HLS INTERFACE m_axi port = in  offset = slave bundle = gmem0 max_read_burst_length =burst_len
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem1 max_write_burst_length=burst_len

#pragma HLS INTERFACE s_axilite port = in  bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control

#pragma HLS INTERFACE s_axilite port = nburst_per_uv bundle = control
#pragma HLS INTERFACE s_axilite port = ntime         bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_dm_per_cu     bundle = control
#pragma HLS INTERFACE s_axilite port = return        bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out

  fifo_cmplx in_fifo;
  fifo_cmplx out_fifo;
  const int nburst_per_tran = BURST_LEN_TRAN*BURST_LEN_TRAN;
  //#pragma HLS STREAM variable = in_fifo  depth = nburst_per_tran //256 //32
  //#pragma HLS STREAM variable = out_fifo depth = nburst_per_tran //256 //32
  
#pragma HLS DATAFLOW
  
  read2fifo(
            nburst_per_uv,
            ntime,
            nburst_dm_per_cu,
            in,
            in_fifo);
  
  transpose(
            nburst_per_uv,
            ntime,
            nburst_dm_per_cu,
            in_fifo,
            out_fifo);

  write_from_fifo(
                  nburst_per_uv,
                  ntime,
                  nburst_dm_per_cu,
                  out_fifo,
                  out);
}

void read2fifo(
               int nburst_per_uv,
               int ntime,
               int nburst_dm_per_cu,
               const burst_cmplx *in,
               fifo_cmplx &in_fifo){
  int i;
  int j;
  int k;
  int m;
  int n;
  int loc_burst;
  int ntran_dm         = nburst_dm_per_cu/BURST_LEN_TRAN;
  int ntran_per_uv     = nburst_per_uv/BURST_LEN_TRAN;
  int ncmplx_per_burst = NCMPLX_PER_BURST;

  const int max_burst_per_uv = MAX_BURST_PER_UV;
  const int max_tran_per_uv  = MAX_BURST_PER_UV/BURST_LEN_TRAN;
  const int max_time  = MAX_TIME;  
  const int max_tran_dm      = MAX_BURST_DM/BURST_LEN_TRAN;
    
  for(k = 0; k < ntime; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_time
    for(i = 0; i < ntran_per_uv; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_per_uv
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_dm
        
        // Burst in
        for(m = 0; m < TILE_WIDTH_TRAN; m++){            // For UV
        loop_read2fifo:
          for(n = 0; n < BURST_LEN_TRAN; n++){        // For DM
#pragma HLS PIPELINE
            loc_burst = (i*TILE_WIDTH_TRAN+m)*ntime*nburst_dm_per_cu+
              k*nburst_dm_per_cu+
              j*BURST_LEN_TRAN +
              n;
            in_fifo.write(in[loc_burst]);
          }
        }        
      }      
    }
  }
}

void transpose(
               int nburst_per_uv,
               int ntime,
               int nburst_dm_per_cu,
               fifo_cmplx &in_fifo,
               fifo_cmplx &out_fifo){
  int i;
  int j;
  int k;
  int ntran_dm     = nburst_dm_per_cu/BURST_LEN_TRAN;
  int ntran_per_uv = nburst_per_uv/BURST_LEN_TRAN;
  
  const int max_tran_per_uv     = MAX_BURST_PER_UV/BURST_LEN_TRAN;
  const int max_time     = MAX_TIME;  
  const int max_tran_dm         = MAX_BURST_DM/BURST_LEN_TRAN;
  const int ncmplx_per_burst  = NCMPLX_PER_BURST;
  
  cmplx_t cmplx_tile[TILE_WIDTH_TRAN][TILE_WIDTH_TRAN];
  //#pragma HLS ARRAY_RESHAPE variable = cmplx_tile    cyclic factor = ncmplx_per_burst dim =1
  //#pragma HLS ARRAY_RESHAPE variable = cmplx_tile    cyclic factor = ncmplx_per_burst dim =2
#pragma HLS ARRAY_PARTITION variable = cmplx_tile    cyclic factor = ncmplx_per_burst dim =1
#pragma HLS ARRAY_PARTITION variable = cmplx_tile    cyclic factor = ncmplx_per_burst dim =2

  for(k = 0; k < ntime; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_time
    
#ifndef __SYNTHESIS__
    fprintf(stdout, "HERE KERNEL_TRAN\t%d\n", k);
#endif
    
    for(i = 0; i < ntran_per_uv; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_per_uv
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_dm
#pragma HLS DATAFLOW 
        
        fill_tile(in_fifo, cmplx_tile);
        transpose_tile(cmplx_tile, out_fifo);
      }      
    }
  }  
}

void fill_tile(
               fifo_cmplx &in_fifo,
               cmplx_t cmplx_tile[TILE_WIDTH_TRAN][TILE_WIDTH_TRAN]){
  
  int m;
  int n;
  int n1;
  int loc_tile;
  burst_cmplx burst;
  
  for(m = 0; m < TILE_WIDTH_TRAN; m++){              // For UV
  loop_fill_tile:
    for(n = 0; n < BURST_LEN_TRAN; n++){          // For DM
#pragma HLS PIPELINE
      burst = in_fifo.read();
      for(n1 = 0; n1 < NCMPLX_PER_BURST; n1++){  // For DM
        loc_tile = n*NCMPLX_PER_BURST+n1;
        cmplx_tile[m][loc_tile] = burst.data[n1];
      }
    }
  }
}

void transpose_tile(
                    cmplx_t cmplx_tile[TILE_WIDTH_TRAN][TILE_WIDTH_TRAN],
                    fifo_cmplx &out_fifo){
  int m;
  int n;
  int n1;
  int loc_tile;
  burst_cmplx burst;
  
  for(m = 0; m < TILE_WIDTH_TRAN; m++){              // For DM
  loop_transpose_tile:
    for(n = 0; n < BURST_LEN_TRAN; n++){          // For UV
#pragma HLS PIPELINE            
      for(n1 = 0; n1 < NCMPLX_PER_BURST; n1++){  // For UV
        loc_tile = n*NCMPLX_PER_BURST+n1;
        burst.data[n1] = cmplx_tile[loc_tile][m];
      }
      out_fifo.write(burst);
    }          
  }
}

void write_from_fifo(
                     int nburst_per_uv,
                     int ntime,
                     int nburst_dm_per_cu,
                     fifo_cmplx &out_fifo,
                     burst_cmplx *out){
  int i;
  int j;
  int k;
  int m;
  int n;
  int loc_burst;
  int ntran_dm     = nburst_dm_per_cu/BURST_LEN_TRAN;
  int ntran_per_uv = nburst_per_uv/BURST_LEN_TRAN;
  
  const int max_tran_per_uv = MAX_BURST_PER_UV/BURST_LEN_TRAN;
  const int max_time = MAX_TIME;  
  const int max_tran_dm     = MAX_BURST_DM/BURST_LEN_TRAN;
  
  for(k = 0; k < ntime; k++){        // For Time
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_time
    for(i = 0; i < ntran_per_uv; i++){  // For UV
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_per_uv
      for(j = 0; j < ntran_dm; j++){        // For DM
#pragma HLS LOOP_TRIPCOUNT min = 1 max = max_tran_dm
        
        // Burst out
        for(m = 0; m < TILE_WIDTH_TRAN; m++){            // For DM
        loop_write_from_fifo:
          for(n = 0; n < BURST_LEN_TRAN; n++){        // For UV
#pragma HLS PIPELINE
            loc_burst = (j*TILE_WIDTH_TRAN+m)*ntime*nburst_per_uv +
              k*nburst_per_uv +
              i*BURST_LEN_TRAN +
              n;
            out[loc_burst] = out_fifo.read();
          }
        }        
      }      
    }
  }
}
