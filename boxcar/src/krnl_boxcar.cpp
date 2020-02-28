#include "boxcar.h"

extern "C"{
  void read_history(
                    burst_real *history,
                    fifo_burst &history_fifo
                    );

  void fifo2history(
                    fifo_burst &history_fifo,
                    real_t history[NPIX2][NBOXCAR]
                    );

  void history2fifo(
                    fifo_burst &history_fifo,
                    real_t history[NPIX2][NBOXCAR]
                    );
  
  void process(
               stream_burst &in, 
               fifo_burst &history0_fifo,
               fifo_burst &history1_fifo,
               cand_t *out);

  void write_history(
                     burst_real *history,
                     fifo_burst &history_fifo
                     );
  
  void boxcar2cand(
                   stream_burst &in,
                   fifo_burst &history0_fifo,
                   fifo_burst &history1_fifo,
                   fifo_cand cand_fifo[NREAL_PER_BURST]);
  
  void boxcar2cand_dm(
                      int dm_index,
                      stream_burst &in,
                      real_t history[NPIX2][NBOXCAR],
                      fifo_cand cand_fifo[NREAL_PER_BURST]);
  
  void boxcar2cand_time(
                        int dm_index,
                        int t,
                        stream_burst &in,
                        real_t history[NPIX2][NBOXCAR],
                        fifo_cand cand_fifo[NREAL_PER_BURST]);
  
  void write_cand(
                  cand_t *out,
                  fifo_cand cand_fifo[NREAL_PER_BURST]);
}

void read_history(
                  burst_real *history,
                  fifo_burst &history_fifo
                  ){
  int i;
  
 loop_read_history:
  for(i = 0; i < NBURST_PER_IMG*NDM*NBOXCAR; i++){
#pragma HLS PIPELINE
    history_fifo.write(history[i]);
  }
}

void process(
             stream_burst &in, 
             fifo_burst &history0_fifo,
             fifo_burst &history1_fifo,
             cand_t *out){
  int i;
  fifo_cand cand_fifo[NREAL_PER_BURST];
  for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
#pragma HLS STREAM variable=cand_fifo[i]
  }
  
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
  for(i = 0; i < NBURST_PER_IMG*NDM*NBOXCAR; i++){
#pragma HLS PIPELINE
    history[i] = history_fifo.read();
  }
}

void boxcar2cand(
                 stream_burst &in,
                 fifo_burst &history0_fifo,
                 fifo_burst &history1_fifo,
                 fifo_cand cand_fifo[NREAL_PER_BURST]){
  int t;
  int dm_index;
  int i;
  real_t history[NPIX2][NBOXCAR];
  const int nreal_per_burst = NREAL_PER_BURST;
  
  //#pragma HLS ARRAY_PARTITION variable=history dim=1 factor=nreal_per_burst cyclic
#pragma HLS ARRAY_RESHAPE variable=history dim=1 factor=nreal_per_burst cyclic
  //#pragma HLS ARRAY_RESHAPE variable=history dim=2 complete
  
 loop_boxcar2cand:
  for(dm_index = 0; dm_index < NDM; dm_index++){
    fifo2history(history0_fifo, history);    
    boxcar2cand_dm(dm_index, in, history, cand_fifo);
    history2fifo(history1_fifo, history);
  }
  
  // write termination to each fifo so the next process will quit
 loop_terminate:
  for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
    cand_fifo[i].write(FINISH_CAND); // FINISH_CAND is the termination marker
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
    for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
      if(cand_fifo[i].read_nb(cand)){ // successfull
        if(cand.snr == -1){ // finished signal
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
                    real_t history[NPIX2][NBOXCAR],
                    fifo_cand cand_fifo[NREAL_PER_BURST]){
  int t;
  
  for(t = 0; t < NTIME; t++){
    boxcar2cand_time(dm_index, t, in, history, cand_fifo);
  }
}

void boxcar2cand_time(
                      int dm_index,
                      int t,
                      stream_burst &in,
                      real_t history[NPIX2][NBOXCAR],
                      fifo_cand cand_fifo[NREAL_PER_BURST]){
  int i;
  int j;
  int w;
  int loc_img;
  stream_burst_core burst_in;
  cand_t cand;
  accum_t boxcar;
  accum_t boxcar_thresh;
  accum_t correction;
  
 loop_img:
  for(i  = 0; i < NBURST_PER_IMG; i++){
#pragma HLS PIPELINE 
    burst_in = in.read();
    
    for(j = 0; j < NREAL_PER_BURST; j++){
      loc_img = i*NREAL_PER_BURST + j;
      
      // Shift data along in the history and put new data in
      for(w = NBOXCAR-1; w >0; w--){
        history[loc_img][w] = history[loc_img][w-1];
      }
      history[loc_img][0] = burst_in.data((j+1)*REAL_WIDTH-1, j*REAL_WIDTH);
      
      // Set initial value and check snr
      cand.snr          = 0;
      cand.boxcar_width = 0;
      cand.loc_img      = loc_img;
      cand.dm_index     = dm_index;
      cand.t            = t;
      boxcar            = 0;      
      for(w = 0; w < NBOXCAR; w++){ // Is b a carried dependance?
        boxcar += history[loc_img][w];
        correction    = 1.0f/sqrtf(w+1);
        boxcar_thresh = boxcar*correction;
        if (boxcar_thresh > cand.snr){
          cand.snr          = boxcar_thresh;
          cand.boxcar_width = w+1;
        }
      }

      // Write cand to fifo
      if(cand.snr >= THRESHOLD){
        cand_fifo[j].write_nb(cand); 
      }
    }
  }
}

// This is the top level function
// reads data from memory 512 bits wide, computes boxcar and writes out candidates
// above the S/N threshold.
void krnl_boxcar2cand(
                      stream_burst &in,
                      burst_real *history0,
                      burst_real *history1,
                      cand_t* out){
  // Output of Alex's FFT will be an hls::stream  512 bits wide I think.
  // Which will send 32 numbers per clock
  // Rastering throught the output image for a single image until it gets to the next image time
  // Then the next DM.
  
#pragma HLS INTERFACE axis      port = in
#pragma HLS INTERFACE m_axi     port = history0 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi     port = history1 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi     port = out      offset = slave bundle = gmem3

#pragma HLS INTERFACE s_axilite port = history0 bundle = control
#pragma HLS INTERFACE s_axilite port = history1 bundle = control
#pragma HLS INTERFACE s_axilite port = return   bundle = control

#pragma HLS DATA_PACK variable = history0
#pragma HLS DATA_PACK variable = history1
#pragma HLS DATA_PACK variable = out
  
  fifo_burst history0_fifo;
  fifo_burst history1_fifo;
#pragma HLS STREAM variable=history0_fifo
#pragma HLS STREAM variable=history1_fifo
#pragma HLS DATAFLOW
  
  read_history(history0, history0_fifo);
  process(in, history0_fifo, history1_fifo, out);
  write_history(history1, history1_fifo);
}

void fifo2history(
                  fifo_burst &history_fifo,
                  real_t history[NPIX2][NBOXCAR]
                  ){  
  int i;
  int j;
  int k;
  int loc;
  burst_real burst;
  
  for(i = 0; i < NBOXCAR; i++){
  loop_fifo2history:
    for(j = 0; j < NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      burst = history_fifo.read();
      for(k = 0; k < NREAL_PER_BURST; k++){
        loc = j*NREAL_PER_BURST+k;
        history[loc][i] = burst.data[k];
      }
    }
  }
}

void history2fifo(
                  fifo_burst &history_fifo,
                  real_t history[NPIX2][NBOXCAR]
                  ){  
  int i;
  int j;
  int k;
  int loc;
  burst_real burst;
  
  for(i = 0; i < NBOXCAR; i++){
  loop_history2fifo:
    for(j = 0; j < NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      for(k = 0; k < NREAL_PER_BURST; k++){
        loc = j*NREAL_PER_BURST+k;
        burst.data[k] = history[loc][i];
      }
      history_fifo.write(burst);
    }
  }
}
