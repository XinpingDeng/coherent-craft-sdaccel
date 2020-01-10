#include "grid.h"

// Order is assumed to be DM-TIME-UV
extern "C" {  
  void knl_grid(
		const burst_uv *in,
		const burst_coord *coord,
		burst_uv *out,
		int nuv_per_cu
		);
}
void fill_buffer(
                 const burst_uv *in,
                 uv_t *buffer,
                 int loc_inburst0,
                 int loc_inburst1,
                 int unused,
                 int fill);

void shift_buffer(
                  uv_t *buffer,
                  int used,
                  int shift);

void shift_buffer3(
                   uv_t *buffer,
                   int unused);

void fill_buffer3(
                  const burst_uv *in,
                  uv_t *buffer,
                  int loc_inburst0,
                  int loc_inburst1,
                  int unused);

void knl_grid(
	      const burst_uv *in,
	      const burst_coord *coord,
	      burst_uv *out,
	      int nuv_per_cu
	      )
{
#pragma HLS INTERFACE m_axi port = in    offset = slave bundle = gmem0 
#pragma HLS INTERFACE m_axi port = coord offset = slave bundle = gmem1 
#pragma HLS INTERFACE m_axi port = out   offset = slave bundle = gmem2 max_write_burst_length=64

#pragma HLS INTERFACE s_axilite port = in         bundle = control
#pragma HLS INTERFACE s_axilite port = coord      bundle = control
#pragma HLS INTERFACE s_axilite port = out        bundle = control
#pragma HLS INTERFACE s_axilite port = nuv_per_cu bundle = control
  
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = coord

  burst_uv out_burst;
  uv_t buffer[NDATA_PER_BUFFER]; 
  coord_t2 coord_start[NSAMP_PER_UV_OUT];
  coord_t3 coord_count[NSAMP_PER_UV_OUT];

  const int nsamp_per_burst = NSAMP_PER_BURST;
#pragma HLS ARRAY_PARTITION variable = buffer complete dim=1
#pragma HLS ARRAY_PARTITION variable = coord_start cyclic factor=nsamp_per_burst
#pragma HLS ARRAY_PARTITION variable = coord_count cyclic factor=nsamp_per_burst

  int i;
  int j;
  int m;
  int n;
  int start;
  int count;
  int end;
  int loc_coord;
  int loc_buffer;
  int loc_inburst1;
  int loc_inburst0;
  int loc_inburst1_tail;
  int loc_outburst;
  int unused;
  
#pragma HLS DEPENDENCE variable = loc_inburst1 intra false 
#pragma HLS DEPENDENCE variable = loc_inburst1 inter false 
  //#pragma HLS DEPENDENCE variable = buffer intra false
  
  // Burst in all coordinate
  // input [start count start count ...]
 LOOP_BURST_COORD:
  for(i = 0; i < NBURST_PER_UV_OUT; i++){
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc_coord = i*NSAMP_PER_BURST+j;
      coord_start[loc_coord] = coord[i].data[2*j];
      coord_count[loc_coord] = coord[i].data[2*j+1];
    }
  }

 LOOP_NUV_PER_CU:
  for(i = 0; i < nuv_per_cu; i++){
    // Assume maximally NBURST_BUFFER input blocks will cover one output block
    // Read in first NBURST_BUFFER blocks of each input UV
    // Put NBURST_BUFFER blocks into a single array to reduce the source usage
  LOOP_BUFFER_IN1:
    for(j = 0; j < NBURST_BUFFER; j++){
#pragma HLS PIPELINE
      loc_inburst1 = j;
      loc_inburst0 = i*NBURST_PER_UV_IN;
      for(m = 0; m < NDATA_PER_BURST; m++){
        buffer[j*NDATA_PER_BURST+m]   = in[loc_inburst0+loc_inburst1].data[m];
      }
    }
    
    loc_inburst1_tail = 0;
  LOOP_SET_UV:
    for(j = 0; j < NBURST_PER_UV_OUT; j++){
#pragma HLS PIPELINE
      //#pragma HLS DATAFLOW
      // Get one output block ready in one clock cycle
      for(m = 0; m < NSAMP_PER_BURST; m++){
        // Default output block elements be 0
        out_burst.data[2*m]   = 0;
        out_burst.data[2*m+1] = 0;

        // If there is data
        loc_coord = j*NSAMP_PER_BURST+m;
        start = coord_start[loc_coord];
        count = coord_count[loc_coord];
        for(n = 0; n < NSAMP_PER_CELL; n++){
          if(n<count){
            loc_buffer = start + n - loc_inburst1_tail*NSAMP_PER_BURST;
            out_burst.data[2*m]   += buffer[2*loc_buffer];
            out_burst.data[2*m+1] += buffer[2*loc_buffer+1];
          }
        }
      }
      loc_outburst      = i*NBURST_PER_UV_OUT+j;
      out[loc_outburst] = out_burst;

      // To see if we need to shift buffer and read in new block
      // Assume that unroll loop returns the last value
      unused = 0;
      for(m = 0; m < NSAMP_PER_BURST; m++){
        loc_coord = j*NSAMP_PER_BURST+m;
        if(coord_count[loc_coord] != 0){
          start = coord_start[loc_coord];          
          count = coord_count[loc_coord];
          end   = start + count;
          loc_inburst1_tail = end/NSAMP_PER_BURST;
        }
      }
      for(m = 0; m < NBURST_BUFFER-1; m++){
        if(loc_inburst1_tail==(loc_inburst1-m)){
          unused = m+1;
        }
      }

      // Shift buffer when necessary
      shift_buffer3(buffer,
                    unused);

      // Fill buffer when necesary
      fill_buffer3(in,
                   buffer,
                   loc_inburst0,
                   loc_inburst1,
                   unused);
      if(unused)
        loc_inburst1 = loc_inburst1+NBURST_BUFFER-unused;
    }
  }
}

void shift_buffer(
                  uv_t *buffer,
                  int used,
                  int shift){
#pragma HLS inline off
  int i;  
  for(i = 0; i < NDATA_PER_BURST; i++){
    // Shift unused block/blocks
    buffer[shift*NDATA_PER_BURST+i] = buffer[(shift+used)*NDATA_PER_BURST+i];
  }
}

void fill_buffer(
                 const burst_uv *in,
                 uv_t *buffer,
                 int loc_inburst0,
                 int loc_inburst1,
                 int unused,
                 int fill){
#pragma HLS inline off
  
  int i;
  
  loc_inburst1 = loc_inburst1+fill+1;      
  for(i = 0; i < NDATA_PER_BURST; i++){
    // Put new block/blocks to array
    buffer[(unused+fill)*NDATA_PER_BURST+i] = in[loc_inburst0+loc_inburst1].data[i];
  }    
}

void shift_buffer3(
                   uv_t *buffer,
                   int unused){
#pragma HLS inline off
  // Shift unused block/blocks to the buffer beginning when necessary
  if(unused==1){
    shift_buffer(buffer,
                 NBURST_BUFFER-unused,
                 0);         
  }
  if(unused==2){
    shift_buffer(buffer,
                 NBURST_BUFFER-unused,
                 0);
    shift_buffer(buffer,
                 NBURST_BUFFER-unused,
                 1); 
  }
}

void fill_buffer3(
                  const burst_uv *in,
                  uv_t *buffer,
                  int loc_inburst0,
                  int loc_inburst1,
                  int unused){
#pragma HLS inline off
  // Put new block/blocks to the buffer end when necessary
  if(unused==1){
    fill_buffer(in,
                buffer,
                loc_inburst0,
                loc_inburst1,
                unused,
                0);
    fill_buffer(in,
                buffer,
                loc_inburst0,
                loc_inburst1,
                unused,
                1);
  }
  if(unused==2){
    fill_buffer(in,
                buffer,
                loc_inburst0,
                loc_inburst1,
                unused,
                0);
  }
}
