#include "boxcar.h"

extern "C"{
  void krnl_boxcar2cand(
                        stream_burst &in,
                        burst_real *history0,
                        burst_real *history1,
                        cand_t *out
                        );
  
  void read_history(
                    burst_real *history,
                    fifo_burst &history_fifo
                    );

  void fifo2history(
                    fifo_burst &history_fifo,
                    burst_real history[NBURST_PER_IMG][NBOXCAR-1]
                    );

  void history2fifo(
                    fifo_burst &history_fifo,
                    burst_real history[NBURST_PER_IMG][NBOXCAR-1]
                    );
  
  void process(
               stream_burst &in, 
               fifo_burst &history0_fifo,
               fifo_burst &history1_fifo,
               cand_t *out
               );
  
  void write_history(
                     burst_real *history,
                     fifo_burst &history_fifo
                     );
  
  void boxcar2cand(
                   stream_burst &in,
                   fifo_burst &history0_fifo,
                   fifo_burst &history1_fifo,
                   fifo_cand cand_fifo[NREAL_PER_BURST]
                   );
  
  void write_cand(
                  cand_t *out,
                  fifo_cand cand_fifo[NREAL_PER_BURST]
                  );
  
  void boxcar2cand_dm(
                      int dm_index,
                      stream_burst &in,
                      accum_t scale_factor[NBOXCAR],
                      burst_real history[NBURST_PER_IMG][NBOXCAR-1],
                      fifo_cand cand_fifo[NREAL_PER_BURST]
                      );

  void boxcar2cand_time(
                        int dm_index,
                        int t, 
                        stream_burst &in,
                        accum_t scale_factor[NBOXCAR],
                        burst_real history[NBURST_PER_IMG][NBOXCAR-1],
                        fifo_cand cand_fifo[NREAL_PER_BURST]
                        );
}

// This is the top level function
// reads data from memory 512 bits wide, computes boxcar and writes out candidates
// above the S/N threshold.
void krnl_boxcar2cand(
                      stream_burst &in,
                      burst_real *history0,
                      burst_real *history1,
                      cand_t *out){
  // Output of Alex's FFT will be an hls::stream  512 bits wide I think.
  // Which will send 32 numbers per clock
  // Rastering throught the output image for a single image until it gets to the next image time
  // Then the next DM.
  
#pragma HLS INTERFACE axis      port = in
#pragma HLS INTERFACE m_axi     port = history0 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi     port = history1 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi     port = out      offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = history0 bundle = control
#pragma HLS INTERFACE s_axilite port = history1 bundle = control
#pragma HLS INTERFACE s_axilite port = out      bundle = control
#pragma HLS INTERFACE s_axilite port = return   bundle = control
  
  fifo_burst history0_fifo;
  fifo_burst history1_fifo;
#pragma HLS DATAFLOW
  
  read_history(history0, history0_fifo);
  process(in, history0_fifo, history1_fifo, out);
  write_history(history1, history1_fifo);
}

void read_history(
                  burst_real *history,
                  fifo_burst &history_fifo
                  ){
  int i;
  
 loop_read_history:
  for(i = 0; i < NBURST_PER_IMG*NDM*(NBOXCAR-1); i++){
#pragma HLS PIPELINE
    history_fifo.write(history[i]);
  }
}

void process(
             stream_burst &in, 
             fifo_burst &history0_fifo,
             fifo_burst &history1_fifo,
             cand_t *out){
  
  fifo_cand cand_fifo[NREAL_PER_BURST];
#pragma HLS DATAFLOW
  boxcar2cand(in, history0_fifo, history1_fifo, cand_fifo);
  write_cand(out, cand_fifo);
}

void write_history(
                   burst_real *history,
                   fifo_burst &history_fifo
                   ){
  int i;

 loop_write_history:
  for(i = 0; i < NBURST_PER_IMG*NDM*(NBOXCAR-1); i++){
#pragma HLS PIPELINE
    history[i] = history_fifo.read();
  }
}

void boxcar2cand(
                 stream_burst &in,
                 fifo_burst &history0_fifo,
                 fifo_burst &history1_fifo,
                 fifo_cand cand_fifo[NREAL_PER_BURST]){
  int i;
  burst_real history[NBURST_PER_IMG][NBOXCAR-1];
  const int nreal_per_burst = NREAL_PER_BURST;
  accum_t scale_factor[NBOXCAR];

  //#pragma HLS ARRAY_PARTITION variable=history dim=2 complete
#pragma HLS ARRAY_RESHAPE variable=history dim=2 complete
  
  // Setup scale factor
#pragma HLS ARRAY_RESHAPE variable=scale_factor
  for(i = 0; i < NBOXCAR; i++){
#pragma HLS UNROLL
    scale_factor[i] = hls::rsqrt(i+1);
  }
  
 loop_boxcar2cand:
  for(i = 0; i < NDM; i++){
    fifo2history(history0_fifo, history);
    boxcar2cand_dm(i, in, scale_factor, history, cand_fifo);
    history2fifo(history1_fifo, history);
  }
  
  // write termination to each fifo so the next process will quit
  cand_t finish_cand;
  finish_cand(0,15) = -1;
 loop_terminate:
  for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
    cand_fifo[i].write(finish_cand); // finishc_cand is the termination marker
  }
}

void write_cand(
                cand_t *out,
                fifo_cand cand_fifo[NREAL_PER_BURST]){
  
  int i;
  int nvalid = NREAL_PER_BURST;
  int ncand  = 0;
  
  cand_t cand;
  
  while(nvalid > 0){
#pragma HLS LOOP_TRIPCOUNT max=1024
    for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
      if(cand_fifo[i].read_nb(cand)){ // successfull
        if(cand(0,15) == -1){ // finished signal
          nvalid--;
        }
        else{
          out[ncand] = cand;
          ncand++;
          if(ncand >= (MAX_CAND-1)){ // avoid overwiting the end of the array
            nvalid = 0; // sends signal to quit.
          }
        }
      }
    }
  }
}

void boxcar2cand_dm(
                    int dm_index,
                    stream_burst &in,
                    accum_t scale_factor[NBOXCAR],
                    burst_real history[NBURST_PER_IMG][NBOXCAR-1],
                    fifo_cand cand_fifo[NREAL_PER_BURST]){
  int i;
  for(i = 0; i < NTIME; i++){
    boxcar2cand_time(dm_index, i, in, scale_factor, history, cand_fifo);
  }
}

void boxcar2cand_time(
                      int dm_index,
                      int t,
                      stream_burst &in,
                      accum_t scale_factor[NBOXCAR],
                      burst_real history[NBURST_PER_IMG][NBOXCAR-1],
                      fifo_cand cand_fifo[NREAL_PER_BURST]){
  int i;
  int j;
  int k;
  int loc_img;
  cand_t cand;
  accum_t boxcar;
  accum_t top_boxcar;
  accum_t scaled_boxcar;
  burst_real tile[NBOXCAR];
  stream_burst_core burst;

#pragma HLS ARRAY_RESHAPE variable=tile
  
  for(i = 0; i < NBURST_PER_IMG; i++){
#pragma HLS PIPELINE
    // Shift and read history, buffer it
    burst = in.read();
    tile[0] = burst.data;
    for(j = 1; j < NBOXCAR; j++){
      tile[j] = history[i][j-1];
    }
    
    // get boxcar
    for(j = 0; j < NREAL_PER_BURST; j++){
      cand    = 0;
      boxcar  = 0;
      loc_img = i*NREAL_PER_BURST+j;
      
      for(k = 0; k < NBOXCAR; k++){
        boxcar       += tile[k]((j+1)*REAL_WIDTH-1, j*REAL_WIDTH);
        scaled_boxcar = boxcar*scale_factor[k];
        if(scaled_boxcar > cand(0,15)){
          cand(0, 15)  = scaled_boxcar;
          cand(16, 31) = loc_img;
          cand(32, 39) = k+1;          
          cand(40, 47) = t;
          cand(48, 63) = dm_index;
        }
      }
      
      // index cand to fifo
      top_boxcar = cand(0, 15);
      if(top_boxcar >= THRESHOLD){
        cand_fifo[j].write_nb(cand); 
      }
    }
    
    // Backup the history
    for(j = 0; j < NBOXCAR-1; j++){
      history[i][j] = tile[j];
    }
  }
}

void fifo2history(
                  fifo_burst &history_fifo,
                  burst_real history[NBURST_PER_IMG][NBOXCAR-1]
                  ){
  int i;
  int j;
  
  for(i = 0; i < NBURST_PER_IMG; i++){
  loop_fifo2history:
    for(j = 0; j < NBOXCAR-1; j++){
#pragma HLS PIPELINE
      history[i][j] = history_fifo.read();
    }
  }
}

void history2fifo(
                  fifo_burst &history_fifo,
                  burst_real history[NBURST_PER_IMG][NBOXCAR-1]
                  ){
  int i;
  int j;
  
  for(i = 0; i < NBURST_PER_IMG; i++){
  loop_fifo2history:
    for(j = 0; j < NBOXCAR-1; j++){
#pragma HLS PIPELINE
      history_fifo.write(history[i][j]);
    }
  }
}
