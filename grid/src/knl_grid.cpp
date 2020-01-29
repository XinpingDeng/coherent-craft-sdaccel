#include "grid.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {  
  void knl_grid(
		const burst_uv *in,
		const burst_coord *coord,
		stream_uv &out_fifo,
		int nuv_per_cu,
                int nburst_per_uv_in,
                int nburst_per_uv_out
		);
  
  void grid(
            int nuv_per_cu,
            int nburst_per_uv_in,
            int nburst_per_uv_out,
            const burst_coord *coord,
            const burst_uv *in,
            stream_uv &out_fifo
            );

  void fill_buffer(
                   int iuv,
                   int nburst_per_uv_in,
                   const burst_uv *in,
                   uv_t *buffer);
  
  void get_loc_in(
                  int iburst,
                  coord_t2 *coord_buffer,
                  int *loc_in
                  );
  
  void fill_out_fifo(
                     int *loc_in,
                     uv_t *buffer,
                     int loc_in_burst,
                     stream_uv &out_fifo
                     );
  
  int get_loc_in_tail(
                      int *loc_in);

  int refill_buffer(
                    int iuv,
                    int nburst_per_uv_in,
                    int loc_in_tail,
                    uv_t *buffer,
                    const burst_uv *in);
  
}

void knl_grid(
	      const burst_uv *in,
	      const burst_coord *coord,
              stream_uv &out_fifo,
	      int nuv_per_cu,
              int nburst_per_uv_in,
              int nburst_per_uv_out
	      )
{
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE axis  port = out_fifo
  
#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_in  bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_uv_out bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control
  
#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = coord
  
  grid(
       nuv_per_cu,
       nburst_per_uv_in,
       nburst_per_uv_out,
       coord,
       in,
       out_fifo
       );
}


void grid(
          int nuv_per_cu,
          int nburst_per_uv_in,
          int nburst_per_uv_out,
          const burst_coord *coord,
          const burst_uv *in,
          stream_uv &out_fifo
          ){
  int i;
  int j;
  int loc_in_burst;
  int loc_in_tail;
  int loc_coord;
  int loc_in[NSAMP_PER_BURST];
  
  const int muv = MUV;
  const int mburst_per_uv_out = MBURST_PER_UV_OUT;

  uv_t buffer[2*NSAMP_PER_BURST];
#pragma HLS ARRAY_RESHAPE variable=buffer complete

  coord_t2 coord_buffer[MSAMP_PER_UV_OUT];
  const int nsamp_per_burst = NSAMP_PER_BURST;
#pragma HLS ARRAY_RESHAPE variable = coord_buffer cyclic factor=nsamp_per_burst
#pragma HLS ARRAY_RESHAPE variable = loc_in complete
  
 loop_burst_coord:
  for(i = 0; i < nburst_per_uv_out; i++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc_coord = i*NSAMP_PER_BURST+j;
      coord_buffer[loc_coord] = coord[i].data[j];
    }
  }
  
  for(i = 0; i < nuv_per_cu; i++){
#pragma HLS LOOP_TRIPCOUNT max = muv
    loc_in_burst = 1;
    fill_buffer(i, nburst_per_uv_in, in, buffer); // Fill two input block into a buffer 
    
  loop_grid:
    for(j = 0; j < nburst_per_uv_out; j++){
#pragma HLS LOOP_TRIPCOUNT max = mburst_per_uv_out
#pragma HLS PIPELINE
      get_loc_in(j, coord_buffer, loc_in);                  // Buffer the coord for current output block    
      fill_out_fifo(loc_in, buffer, loc_in_burst, out_fifo);// Get the current output buffer block
      loc_in_tail = get_loc_in_tail(loc_in);                // Get the tail of the current output buffer block
      if((loc_in_tail/NSAMP_PER_BURST)>=loc_in_burst){      // Refill the buffer when necessary
        loc_in_burst = refill_buffer(i, nburst_per_uv_in, loc_in_tail, buffer, in);
      }
    }  
  }
}

void fill_buffer(
                 int iuv,
                 int nburst_per_uv_in,
                 const burst_uv *in,
                 uv_t *buffer){
  int i;
  int loc0;
  int loc1;
  burst_uv burst0;
  burst_uv burst1;
#pragma HLS DATA_PACK variable=burst0
#pragma HLS DATA_PACK variable=burst1
  
  // Buffer two blocks, two will cover one output block
  loc0 = iuv*nburst_per_uv_in;
  loc1 = iuv*nburst_per_uv_in + 1;
  burst0 = in[loc0];
  burst1 = in[loc1];
  for(i = 0; i < NSAMP_PER_BURST; i++){
#pragma HLS UNROLL
    buffer[i]                 = burst0.data[i];
    buffer[NSAMP_PER_BURST+i] = burst1.data[i];
  }
}

void get_loc_in(
                int iburst,
                coord_t2 *coord_buffer,
                int *loc_in
                ){
  int i;
  int loc = iburst*NSAMP_PER_BURST;
  
  for(i = 0; i < NSAMP_PER_BURST; i++){
    loc_in[i] = coord_buffer[loc+i];
  }  
}

int get_loc_in_tail(
                    int *loc_in){
  int i;
  int loc_in_tail = 0;
  
  for(i = NSAMP_PER_BURST - 1; i >= 0; i--){
    if(loc_in[i] != 0){
      loc_in_tail = loc_in[i];
      break;
    }
  }
  return loc_in_tail;
}

void fill_out_fifo(
                   int *loc_in,
                   uv_t *buffer,
                   int loc_in_burst,
                   stream_uv &out_fifo
                   ){
  int i;
  int loc_buffer;
  stream_t out_burst;
  ap_uint<BURST_WIDTH> burst=0;
  
  // Get one output block ready in one clock cycle
  for(i = 0; i < NSAMP_PER_BURST; i++){    
    // If there is data
    if(loc_in[i] != 0){ 
      loc_buffer        = (loc_in[i] -1 - (loc_in_burst - 1)*NSAMP_PER_BURST)%(2*NSAMP_PER_BURST);
      burst(2*(i+1)*DATA_WIDTH-1, 2*i*DATA_WIDTH) = buffer[loc_buffer];
    }
  }
  out_burst.data = burst;
  out_fifo.write(out_burst);
}

int refill_buffer(
                  int iuv,
                  int nburst_per_uv_in,
                  int loc_in_tail,
                  uv_t *buffer,
                  const burst_uv *in){
  int i;
  int loc_in_burst;
  burst_uv burst;
  
#pragma HLS DATA_PACK variable=burst
  
  loc_in_burst  = loc_in_tail/NSAMP_PER_BURST+1;	
  for(i = 0; i < NSAMP_PER_BURST; i++){
    // Shift the array with one block size
    buffer[i] = buffer[NSAMP_PER_BURST+i];
  }
  burst = in[iuv*nburst_per_uv_in + loc_in_burst];
  for(i = 0; i < NSAMP_PER_BURST; i++){
    // Put the new block into the array 
    buffer[NSAMP_PER_BURST+i] = burst.data[i];
  }
  
  return loc_in_burst;
}
