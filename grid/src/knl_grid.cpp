#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		const burst_uv *in,
		const burst_coord *coord,
		//stream_uv &out_stream,
                burst_uv *out,
		int nuv_per_cu,
                int nburst_per_uv_in,
                int nburst_per_uv_out
		);

  void read_in(
               int nuv_per_cu,
               int nburst_per_uv_in,
               const burst_uv *in,
               fifo_uv &in_fifo);
  
  void write_out(
                 int nuv_per_cu,
                 int nburst_per_uv_out,
                 burst_uv *out,
                 fifo_uv &out_fifo);
  
  void grid(
            int nuv_per_cu,
            int nburst_per_uv_in,
            int nburst_per_uv_out,
            const burst_coord *coord,
            fifo_uv &in_fifo,
            //stream_uv &out_stream
            fifo_uv &out_fifo
            );
  
  void read_coord(
                  int nburst_per_uv_in,
                  const burst_coord *coord,
                  coord_t *coord_buffer);
  
  void set_grid_bool(
                     int nburst_per_uv_in,
                     int nburst_per_uv_out,
                     coord_t *coord_buffer,
                     bool grid_bool[MBURST_PER_UV_OUT][NSAMP_PER_BURST]);
  
  void fill_buffer(
                   fifo_uv &in_fifo,
                   int nburst_per_uv_in,
                   uv_t *buffer
                   );
  
  void buffer2grid(
                   int nburst_per_uv_in,
                   coord_t *coord_buffer,
                   uv_t *buffer,
                   uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST]
                   );
  
  void stream_grid(
                   uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST],
                   int nburst_per_uv_out,
                   bool grid_bool[MBURST_PER_UV_OUT][NSAMP_PER_BURST],
                   //stream_uv &out_stream
                   fifo_uv &out_fifo
                   );
}

void knl_grid(
	      const burst_uv *in,
	      const burst_coord *coord,
              //stream_uv &out_stream,
              burst_uv *out,
	      int nuv_per_cu,
              int nburst_per_uv_in,
              int nburst_per_uv_out
	      )
{
  const int burst_length = BURST_LENGTH;
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 max_read_burst_length=burst_length
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 max_read_burst_length=burst_length
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_write_burst_length=burst_length
  //#pragma HLS INTERFACE axis  port = out_stream
  
#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = out        bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_in  bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control
  
#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = coord

  fifo_uv in_fifo;
  fifo_uv out_fifo;
#pragma HLS STREAM variable=in_fifo
#pragma HLS STREAM variable=out_fifo
#pragma HLS DATAFLOW
  
  read_in(
          nuv_per_cu,
          nburst_per_uv_in,
          in,
          in_fifo);
  
  grid(
       nuv_per_cu,
       nburst_per_uv_in,
       nburst_per_uv_out,
       coord,
       in_fifo,
       //out_stream
       out_fifo
       );

  write_out(
            nuv_per_cu,
            nburst_per_uv_out,
            out,
            out_fifo);
}

void read_in(
             int nuv_per_cu,
             int nburst_per_uv_in,
             const burst_uv *in,
             fifo_uv &in_fifo){
  const int muv = MUV;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;

  int i;
  int j;
  int loc;

  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max=muv
  loop_read_in:
    for(j = 0; j < nburst_per_uv_in; j++){
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_in
#pragma HLS PIPELINE
      loc = i*nburst_per_uv_in + j;
      in_fifo.write(in[loc]);
    }
  }
}

void grid(
          int nuv_per_cu,
          int nburst_per_uv_in,
          int nburst_per_uv_out,
          const burst_coord *coord,
          fifo_uv &in_fifo,
          fifo_uv &out_fifo
          //stream_uv &out_stream
          ){
  int i;
  
  const int muv = MUV;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;

  uv_t buffer[MSAMP_PER_UV_IN];
  uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST];
  coord_t coord_buffer[MSAMP_PER_UV_IN];
  bool grid_bool[MBURST_PER_UV_OUT][NSAMP_PER_BURST];
  
  const int nsamp_per_burst = NSAMP_PER_BURST;
  
  //#pragma HLS ARRAY_RESHAPE variable = grid complete dim =2
  //#pragma HLS ARRAY_RESHAPE variable = grid_bool  complete dim =2
  //#pragma HLS ARRAY_RESHAPE variable = buffer cyclic factor = nsamp_per_burst
  //#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor = nsamp_per_burst
  
#pragma HLS ARRAY_PARTITION variable = grid complete dim =2
#pragma HLS ARRAY_PARTITION variable = grid_bool  complete dim =2
#pragma HLS ARRAY_PARTITION variable = buffer cyclic factor = nsamp_per_burst
#pragma HLS ARRAY_PARTITION variable = coord_buffer cyclic factor = nsamp_per_burst
  
  //read_coord(nburst_per_uv_in, coord, coord_buffer);
  //set_grid_bool(nburst_per_uv_in, nburst_per_uv_out, coord_buffer, grid_bool);
  
  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max = muv
#pragma HLS DATAFLOW
    fill_buffer(in_fifo, nburst_per_uv_in, buffer);
    buffer2grid(nburst_per_uv_in, coord_buffer, buffer, grid);
    //stream_grid(grid, nburst_per_uv_out, grid_bool, out_stream);
    stream_grid(grid, nburst_per_uv_out, grid_bool, out_fifo);
#ifdef __SYNTHESIS__
    fprintf(stdout, "HERE\t%d\n", i);
#endif
  }
}

void read_coord(
                int nburst_per_uv_in,
                const burst_coord *coord,
                coord_t *coord_buffer){
  int i;
  int j;
  int loc;

  const int mburst_per_uv_in = MBURST_PER_UV_IN;
  
 loop_read_coord:
  for(i = 0; i < nburst_per_uv_in; i++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_in
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc = i*NSAMP_PER_BURST+j;
      coord_buffer[loc] = coord[i].data[j];
    }
  }  
}

void fill_buffer(
                 fifo_uv &in_fifo,
                 int nburst_per_uv_in,
                 uv_t *buffer
                 ){
  int i;
  int j;
  int loc;
  burst_uv burst;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;
  
 loop_fill_buffer:
  for(i = 0; i < nburst_per_uv_in; i++){
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_in
#pragma HLS PIPELINE
    
    burst = in_fifo.read();
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc = i*NSAMP_PER_BURST+j;
      buffer[loc] = burst.data[j];
    }
  }
}

/*
void buffer2grid(
                 int nburst_per_uv_in,
                 coord_t *coord_buffer,
                 uv_t *buffer,
                 uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST]
                 ){
  int i;
  uint loc_i;
  uint loc_j;
  uint coord;
  const int msamp_per_uv_in = MSAMP_PER_UV_IN;
  
 loop_buffer2grid:
  for(i = 0; i < nburst_per_uv_in*NSAMP_PER_BURST; i++){
#pragma HLS LOOP_TRIPCOUNT max = msamp_per_uv_in
#pragma HLS PIPELINE
    coord = coord_buffer[i];
    loc_i = coord/NSAMP_PER_BURST;
    loc_j = coord%NSAMP_PER_BURST;
    grid[loc_i][loc_j] = buffer[i];
  }
}
*/

void buffer2grid(
                 int nburst_per_uv_in,
                 uv_t *buffer,
                 uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST]
                 ){
  int i;
  int j;
  uint loc_i;
  uint loc_j;
  uint coord;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;
  
 loop_buffer2grid:
  for(i = 0; i < nburst_per_uv_in; i++){
#pragma HLS LOOP_TRIPCOUNT max = msamp_per_uv_in
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc = i*NSAMP_PER_BURST+j;
      grid[i][j] = buffer[loc];
    }
  }
}

void stream_grid(
                 uv_t grid[MBURST_PER_UV_OUT][NSAMP_PER_BURST],
                 int nburst_per_uv_out,
                 bool grid_bool[MBURST_PER_UV_OUT][NSAMP_PER_BURST],
                 //stream_uv &out_stream
                 fifo_uv &out_fifo
                 ){
  int i;
  int j;
  
  //ap_uint<BURST_WIDTH> burst;
  //stream_t stream;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  burst_uv burst;
  
 loop_grid:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      //burst(2*(j+1)*DATA_WIDTH-1, 2*j*DATA_WIDTH) = grid_bool[i][j]*grid[i][j];
      burst.data[j] = grid_bool[i][j]*grid[i][j];
    }
    
    //stream.data = burst;
    //out_fifo.write(stream);
    out_fifo.write(burst);
  }  
}

void set_grid_bool(
                   int nburst_per_uv_in,
                   int nburst_per_uv_out,
                   coord_t *coord_buffer,
                   bool grid_bool[MBURST_PER_UV_OUT][NSAMP_PER_BURST]){  
  int i;
  int j;
  uint loc_i;
  uint loc_j;
  uint coord;

  const int msamp_per_uv_in = MSAMP_PER_UV_IN;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;

 loop_reset_grid_bool:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
    for(j = 0; j < NSAMP_PER_BURST; j++){
      grid_bool[i][j] = false;
    }
  }

 loop_set_grid_bool:
  for(i = 0; i < nburst_per_uv_in*NSAMP_PER_BURST; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max = msamp_per_uv_in
    coord = coord_buffer[i];
    loc_i = coord/NSAMP_PER_BURST;
    loc_j = coord%NSAMP_PER_BURST;
    grid_bool[loc_i][loc_j] = true;
  }
}


void write_out(
               int nuv_per_cu,
               int nburst_per_uv_out,
               burst_uv *out,
               fifo_uv &out_fifo){
  const int muv = MUV;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  
  int i;
  int j;
  int loc;
  
  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max=muv
  loop_write_out:
    for(j = 0; j < nburst_per_uv_out; j++){
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_out
#pragma HLS PIPELINE
      loc = i*nburst_per_uv_out + j;
      out[loc] = out_fifo.read();
    }
  }
}
