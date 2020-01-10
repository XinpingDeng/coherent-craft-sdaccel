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
  burst_uv in_burst;
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
  int loc_inburst_tail;
  int loc_outburst;
  int unused;
  
#pragma HLS DEPENDENCE variable = loc_inburst1 intra false //true //
#pragma HLS DEPENDENCE variable = loc_inburst1 inter false //true //
  
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
      in_burst = in[loc_inburst0+loc_inburst1];
      for(m = 0; m < NDATA_PER_BURST; m++){
        buffer[j*NDATA_PER_BURST+m] = in_burst.data[m];
      }
    }
    
    loc_inburst_tail = 0;
  LOOP_SET_UV:
    for(j = 0; j < NBURST_PER_UV_OUT; j++){
#pragma HLS PIPELINE  
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
            loc_buffer = start + n - loc_inburst_tail*NSAMP_PER_BURST;
            out_burst.data[2*m]   += buffer[2*loc_buffer];
            out_burst.data[2*m+1] += buffer[2*loc_buffer+1];
          }
        }
      }
      loc_outburst      = i*NBURST_PER_UV_OUT+j;
      out[loc_outburst] = out_burst;

      // Check buffer status
      unused  = 0;
      for(m = 0; m < NSAMP_PER_BURST; m++){
        loc_coord = j*NSAMP_PER_BURST+m;
        if(coord_count[loc_coord] != 0){
          start = coord_start[loc_coord];          
          count = coord_count[loc_coord];
          end   = start + count;
          loc_inburst_tail = end/NSAMP_PER_BURST;
        }
      }      
      for(m = 0; m < NBURST_BUFFER-1; m++){
        if(loc_inburst_tail==(loc_inburst1-m)){
          unused = m+1;
        }
      }

      // shift used block/blocks when necessary
      if(unused==1){
        for(m = 0; m < NDATA_PER_BURST; m++){
          buffer[m] = buffer[2*NDATA_PER_BURST+m];
        }
      }
      if(unused==2){
        for(m = 0; m < NDATA_PER_BURST; m++){
          buffer[m]                 = buffer[NDATA_PER_BURST+m];
          buffer[NDATA_PER_BURST+m] = buffer[2*NDATA_PER_BURST+m];
        }
      }
      // Buffer new block when necessary
      if(unused==1){
        loc_inburst1 = loc_inburst_tail+1;
        in_burst = in[loc_inburst0+loc_inburst1];
        for(m = 0; m < NDATA_PER_BURST; m++){
          buffer[NDATA_PER_BURST+m] = in_burst.data[m];
        }
        loc_inburst1 = loc_inburst_tail+2;
        in_burst = in[loc_inburst0+loc_inburst1];
        for(m = 0; m < NDATA_PER_BURST; m++){
          buffer[2*NDATA_PER_BURST+m] = in_burst.data[m];
        }
      }
      if(unused==2){
        loc_inburst1 = loc_inburst_tail+2;
        in_burst = in[loc_inburst0+loc_inburst1];
        for(m = 0; m < NDATA_PER_BURST; m++){
          buffer[2*NDATA_PER_BURST+m] = in_burst.data[m];
        }
      }
    }    
  }
}
