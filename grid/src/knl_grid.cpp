#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		const burst_uv *in,
		const burst_coord *coord,
		stream_uv &out_stream,
		int nuv_per_cu,
                int nburst_per_uv_in,
                int nburst_per_uv_out
		);

  void read2fifo(
                 int nuv_per_cu,
                 int nburst_per_uv_in,
                 const burst_uv *in,
                 fifo_uv &in_fifo);
  
  void grid(
            int nuv_per_cu,
            int nburst_per_uv_in,
            int nburst_per_uv_out,
            const burst_coord *coord,
            fifo_uv &in_fifo,
            stream_uv &out_stream
            );
  
  void read_coord(
                  int nburst_per_uv_out,
                  const burst_coord *coord,
                  coord_t2 *coord_buffer);
  
  void fill_buffer(
                   fifo_uv &in_fifo,
                   int nburst_per_uv_in,
                   uv_t buffer[MBURST_PER_UV_IN][NSAMP_PER_BURST]
                   );

  void buffer2grid(
                   uv_t buffer[MBURST_PER_UV_IN][NSAMP_PER_BURST],
                   coord_t2 *coord_buffer,
                   int nburst_per_uv_out,
                   stream_uv &out_stream
                   );
  }

void knl_grid(
	      const burst_uv *in,
	      const burst_coord *coord,
              stream_uv &out_stream,
	      int nuv_per_cu,
              int nburst_per_uv_in,
              int nburst_per_uv_out
	      )
{
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE axis  port = out_stream
  
#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_in  bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control
  
#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = coord

  fifo_uv in_fifo;
#pragma HLS STREAM variable=in_fifo
#pragma HLS DATAFLOW
  
  read2fifo(
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
       out_stream
       );
}

void read2fifo(
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
  loop_read2fifo:
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
          stream_uv &out_stream
          ){
  int i;
  
  const int muv = MUV;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;

  uv_t buffer[MBURST_PER_UV_IN][NSAMP_PER_BURST];
  coord_t2 coord_buffer[MSAMP_PER_UV_OUT];
  const int nsamp_per_burst = NSAMP_PER_BURST;
#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor = nsamp_per_burst
  //#pragma HLS ARRAY_RESHAPE variable = buffer cyclic factor = nsamp_per_burst
  //#pragma HLS ARRAY_PARTITION variable = buffer complete dim = 1
#pragma HLS ARRAY_PARTITION variable = buffer cyclic factor = nsamp_per_burst dim = 1
#pragma HLS ARRAY_PARTITION variable = buffer complete dim = 2
  
  read_coord(nburst_per_uv_out, coord, coord_buffer);
  
  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max = muv
#pragma HLS DATAFLOW
    fill_buffer(in_fifo, nburst_per_uv_in, buffer);     
    buffer2grid(buffer, coord_buffer, nburst_per_uv_out, out_stream);
  }
}

void read_coord(
                int nburst_per_uv_out,
                const burst_coord *coord,
                coord_t2 *coord_buffer){
  int i;
  int j;
  int loc;

  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
  
 loop_read_coord:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
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
                 uv_t buffer[MBURST_PER_UV_IN][NSAMP_PER_BURST]
                 ){
  int i;
  int j;
  //int loc;
  burst_uv burst;
  const int mburst_per_uv_in = MBURST_PER_UV_IN;

 loop_fill_buffer:
  for(i = 0; i < nburst_per_uv_in; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max=mburst_per_uv_in
    
    burst = in_fifo.read();
    for(j = 0; j < NSAMP_PER_BURST; j++){
      //loc = i * NSAMP_PER_BURST + j;
      //buffer[loc] = burst.data[j];
      buffer[i][j] = burst.data[j];
    }
  }
}

void buffer2grid(
                 uv_t buffer[MBURST_PER_UV_IN][NSAMP_PER_BURST],
                 coord_t2 *coord_buffer,
                 int nburst_per_uv_out,
                 stream_uv &out_stream
                 ){
  int i;
  int j;
  uint coord;
  int loc_coord;
  uint loc_i;
  uint loc_j;
  
  ap_uint<BURST_WIDTH> burst;
  stream_t stream;
  
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;
 loop_grid:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc_coord = i*NSAMP_PER_BURST+j;
      coord = coord_buffer[loc_coord];
      //if(coord==0){
      //burst(2*(j+1)*DATA_WIDTH-1, 2*j*DATA_WIDTH) = 0;
      //}
      //else{
      if(coord!=0){
        loc_i = (coord-1)/NSAMP_PER_BURST;
        loc_j = (coord-1)%NSAMP_PER_BURST;
        burst(2*(j+1)*DATA_WIDTH-1, 2*j*DATA_WIDTH) = buffer[loc_i][loc_j];
      }
    }
    stream.data = burst;
    out_stream.write(stream);
  }  
}
