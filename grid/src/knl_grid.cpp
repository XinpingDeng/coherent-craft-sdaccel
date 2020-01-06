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
  uv_t in_buffer[NBURST_BUFFER*NDATA_PER_BURST]; 
  coord_t2 coord_start[NSAMP_PER_UV_OUT];
  coord_t3 coord_count[NSAMP_PER_UV_OUT];

  const int nsamp_per_burst = NSAMP_PER_BURST;
#pragma HLS ARRAY_PARTITION variable = in_buffer complete
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
  int loc_in_buffer;
  int loc_in_burst;
  int loc_in_burst0;
  int loc_end_burst;
  int loc_out_burst;
  int shift;
  int remind;

  FILE *fp = NULL;
  fp = fopen("/data/FRIGG_2/Workspace/coherent-craft-sdaccel/grid/src/knl_error.txt", "w");
  
#pragma HLS DEPENDENCE variable = loc_in_burst intra true //false 
#pragma HLS DEPENDENCE variable = loc_in_burst inter true //false 
  
  // Burst in all coordinate
  // input [start count start count ...]
 LOOP_BURST_COORD:
  for(i = 0; i < NBURST_PER_UV_OUT; i++){
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
#pragma HLS UNROLL
      loc_coord = i*NSAMP_PER_BURST+j;
      coord_start[loc_coord] = coord[i].data[2*j];
      coord_count[loc_coord] = coord[i].data[2*j+1];
    }
  }
  
  for(i = 0; i < nuv_per_cu; i++){
    // Assume maximally NBURST_BUFFER input blocks will cover one output block
    // Read in first NBURST_BUFFER blocks of each input UV
    // Put NBURST_BUFFER blocks into a single array to reduce the source usage
  LOOP_BUFFER_IN1:
    for(j = 0; j < NBURST_BUFFER; j++){
#pragma HLS PIPELINE
      loc_in_burst  = j;
      loc_in_burst0 = i*NBURST_PER_UV_IN;
      for(m = 0; m < NSAMP_PER_BURST; m++){
#pragma HLS UNROLL
        in_buffer[j*NDATA_PER_BURST+2*m]   = in[loc_in_burst0+loc_in_burst].data[2*m];
        in_buffer[j*NDATA_PER_BURST+2*m+1] = in[loc_in_burst0+loc_in_burst].data[2*m+1];
      }
    }

    loc_end_burst = 0;
  LOOP_SET_UV:
    for(j = 0; j < NBURST_PER_UV_OUT; j++){
#pragma HLS PIPELINE  
      // Get one output block ready in one clock cycle
      for(m = 0; m < NSAMP_PER_BURST; m++){
#pragma HLS UNROLL
        // Default output block elements be 0
        out_burst.data[2*m]   = 0;
        out_burst.data[2*m+1] = 0;

        // If there is data
        loc_coord = j*NSAMP_PER_BURST+m;
        start = coord_start[loc_coord];
        count = coord_count[loc_coord];
        for(n = 0; n < NSAMP_PER_CELL; n++){
          if(n<count){
            loc_in_buffer = (start + n - loc_end_burst*NSAMP_PER_BURST);//%(NBURST_BUFFER*NSAMP_PER_BURST);
            if(!((start+1)%NSAMP_PER_BURST))
              fprintf(fp, "%d\t%d\t%d\t%d\t%d\n", start, n, loc_in_burst, start + n - (loc_in_burst + 1 - NBURST_BUFFER)*NSAMP_PER_BURST, loc_in_buffer);
            
            out_burst.data[2*m]   += in_buffer[2*loc_in_buffer];
            out_burst.data[2*m+1] += in_buffer[2*loc_in_buffer+1];
          }
        }
      }
      loc_out_burst      = i*NBURST_PER_UV_OUT+j;
      out[loc_out_burst] = out_burst;
      
    LOOP_GET_END:
      for(m = 0; m < NSAMP_PER_BURST; m++){
        loc_coord = j*NSAMP_PER_BURST+m;
        if(coord_count[loc_coord] != 0){
          start = coord_start[loc_coord];          
          count = coord_count[loc_coord];
          end   = start + count;
          loc_end_burst = end/NSAMP_PER_BURST;
        }
      }

    LOOP_GET_SHIFT_REMIND:
      shift  = 0;
      remind = 0;
      for(m = 0; m < NBURST_BUFFER-1; m++){
        if((loc_end_burst<(loc_in_burst-m+1))&&
           (loc_end_burst>=(loc_in_burst-m))){
          shift  = m+1;
          remind = NBURST_BUFFER-shift;
        }
      }

    LOOP_SHIFT:
      for(m = 0; m < shift; m++){
        for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
          // Shift remind block/blocks               
          in_buffer[m*NDATA_PER_BURST+2*n]   = in_buffer[(m+remind)*NDATA_PER_BURST+2*n];
          in_buffer[m*NDATA_PER_BURST+2*n+1] = in_buffer[(m+remind)*NDATA_PER_BURST+2*n+1];
        }
      }

    LOOP_BUFFER_IN2:
      for(m = 0; m < remind; m++){
        loc_in_burst = loc_end_burst+m+shift;
        for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
          // Put new block/blocks to array 
          in_buffer[(m+shift)*NDATA_PER_BURST+2*n]   = in[loc_in_burst0+loc_in_burst].data[2*n];
          in_buffer[(m+shift)*NDATA_PER_BURST+2*n+1] = in[loc_in_burst0+loc_in_burst].data[2*n+1];
        }        
      }
    }    
  }
  fclose(fp);
}
