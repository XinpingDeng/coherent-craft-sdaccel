#include "krnl.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void krnl_grid(
                 const burst_cmplx *in,
                 const burst_coord *coord,
                 stream_cmplx &out_stream,
                 int nplane,
                 int nburst_per_uv,
                 int nburst_per_img
                 );

  void read_in(
               int nplane,
               int nburst_per_uv,
               const burst_cmplx *in,
               fifo_cmplx &in_fifo);
  
  void grid(
            int nplane,
            int nburst_per_uv,
            int nburst_per_img,
            const burst_coord *coord,
            fifo_cmplx &in_fifo,
            stream_cmplx &out_stream
            );
  
  void read_coord(
                  int nburst_per_uv,
                  const burst_coord *coord,
                  coord_t *coord_buffer);
    
  void fill_buffer(
                   fifo_cmplx &in_fifo,
                   int nburst_per_uv,
                   cmplx_t *buffer
                   );
  
  void buffer2grid(
                   int nburst_per_uv,
                   coord_t *coord_buffer,
                   cmplx_t *buffer,
                   cmplx_t grid[MAX_BURST_PER_IMG][NCMPLX_PER_BURST]
                   );
  
  void stream_grid(
                   cmplx_t grid[MAX_BURST_PER_IMG][NCMPLX_PER_BURST],
                   int nburst_per_img,
                   stream_cmplx &out_stream
                   );
}

void krnl_grid(
               const burst_cmplx *in,
               const burst_coord *coord,
               stream_cmplx &out_stream,
               int nplane,
               int nburst_per_uv,
               int nburst_per_img
               ){
  const int burst_len = BURST_LEN_GRID;
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 max_read_burst_length = burst_len
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 max_read_burst_length = burst_len
#pragma HLS INTERFACE axis  port = out_stream
  
#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = nplane bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv  bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_img bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control
  
#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = coord

  fifo_cmplx in_fifo;
#pragma HLS STREAM variable=in_fifo
#pragma HLS DATAFLOW
  
  read_in(
          nplane,
          nburst_per_uv,
          in,
          in_fifo);
  
  grid(
       nplane,
       nburst_per_uv,
       nburst_per_img,
       coord,
       in_fifo,
       out_stream
       );
}

void read_in(
             int nplane,
             int nburst_per_uv,
             const burst_cmplx *in,
             fifo_cmplx &in_fifo){
  const int max_uv = MAX_PLANE;
  const int max_burst_per_uv = MAX_BURST_PER_UV;

  int i;
  int j;
  int loc;

  for(i = 0; i < nplane; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_uv
  loop_read_in:
    for(j = 0; j < nburst_per_uv; j++){
#pragma HLS LOOP_TRIPCOUNT max=max_burst_per_uv
#pragma HLS PIPELINE
      loc = i*nburst_per_uv + j;
      in_fifo.write(in[loc]);
    }
  }
}

void grid(
          int nplane,
          int nburst_per_uv,
          int nburst_per_img,
          const burst_coord *coord,
          fifo_cmplx &in_fifo,
          stream_cmplx &out_stream
          ){
  int i;
  
  const int max_plane = MAX_PLANE;
  const int max_burst_per_img = MAX_BURST_PER_IMG;
  const int max_burst_per_uv = MAX_BURST_PER_UV;

  cmplx_t buffer[MAX_SMP_PER_UV];
  cmplx_t grid[MAX_BURST_PER_IMG][NCMPLX_PER_BURST];
  coord_t coord_buffer[MAX_SMP_PER_UV];
  
  const int ncmplx_per_burst = NCMPLX_PER_BURST;
  
  //#pragma HLS ARRAY_RESHAPE variable = grid complete dim =2
  //#pragma HLS ARRAY_RESHAPE variable = buffer cyclic factor = ncmplx_per_burst
  //#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor = ncmplx_per_burst
  
#pragma HLS ARRAY_PARTITION variable = grid complete dim =2
#pragma HLS ARRAY_PARTITION variable = buffer cyclic factor = ncmplx_per_burst
#pragma HLS ARRAY_PARTITION variable = coord_buffer cyclic factor = ncmplx_per_burst
  
  read_coord(nburst_per_uv, coord, coord_buffer);
  
  for(i = 0; i < nplane; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_plane
#pragma HLS DATAFLOW
    fill_buffer(in_fifo, nburst_per_uv, buffer);
    buffer2grid(nburst_per_uv, coord_buffer, buffer, grid);
    stream_grid(grid, nburst_per_img, out_stream);
#ifndef __SYNTHESIS__
    fprintf(stdout, "HERE KERNEL_GRID\t%d\n", i);
#endif
  }
}

void read_coord(
                int nburst_per_uv,
                const burst_coord *coord,
                coord_t *coord_buffer){
  int i;
  int j;
  int loc;

  const int max_burst_per_uv = MAX_BURST_PER_UV;
  
 loop_read_coord:
  for(i = 0; i < nburst_per_uv; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_burst_per_uv
#pragma HLS PIPELINE
    for(j = 0; j < NCOORD_PER_BURST; j++){
      loc = i*NCOORD_PER_BURST+j;
      coord_buffer[loc] = coord[i].data[j];
    }
  }  
}

void fill_buffer(
                 fifo_cmplx &in_fifo,
                 int nburst_per_uv,
                 cmplx_t *buffer
                 ){
  int i;
  int j;
  int loc;
  burst_cmplx burst;
  const int max_burst_per_uv = MAX_BURST_PER_UV;
  
 loop_fill_buffer:
  for(i = 0; i < nburst_per_uv; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_burst_per_uv
#pragma HLS PIPELINE
    
    burst = in_fifo.read();
    for(j = 0; j < NCMPLX_PER_BURST; j++){
      loc = i*NCMPLX_PER_BURST+j;
      buffer[loc] = burst.data[j];
    }
  }
}

void buffer2grid(
                 int nburst_per_uv,
                 coord_t *coord_buffer,
                 cmplx_t *buffer,
                 cmplx_t grid[MAX_BURST_PER_IMG][NCMPLX_PER_BURST]
                 ){
  int i;
  uint loc_i;
  uint loc_j;
  uint coord;
  const int msmp_per_uv = MAX_SMP_PER_UV;
  
 loop_buffer2grid:
  for(i = 0; i < nburst_per_uv*NCMPLX_PER_BURST; i++){
#pragma HLS LOOP_TRIPCOUNT max = msmp_per_uv
#pragma HLS PIPELINE
    coord = coord_buffer[i];
    loc_i = coord/NCMPLX_PER_BURST;
    loc_j = coord%NCMPLX_PER_BURST;
    grid[loc_i][loc_j] = buffer[i];
  }
}

void stream_grid(
                 cmplx_t grid[MAX_BURST_PER_IMG][NCMPLX_PER_BURST],
                 int nburst_per_img,
                 stream_cmplx &out_stream
                 ){
  int i;
  int j;
  uint coord;
  int loc_coord;
  
  stream_cmplx_core stream;
  const int max_burst_per_img = MAX_BURST_PER_IMG;

 loop_grid:
  for(i = 0; i < nburst_per_img; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_burst_per_img
#pragma HLS PIPELINE
    for(j = 0; j < NCMPLX_PER_BURST; j++){
      stream.data((j+1)*CMPLX_WIDTH-1, j*CMPLX_WIDTH) = grid[i][j];
    }
    out_stream.write(stream);
  }  
}
