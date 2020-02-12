#include "boxcar.h"

extern "C"{
  void krnl_box(
                fifo_burst_t &in,
                burst_t *previous_history,
                burst_t *current_history,
                int ndm,
                int ntime,
                data_t threshold,
                data_t *out
                );
  
  void fill_cand_fifo(
                      fifo_burst_t &in, 
                      burst_t *previous_history,
                      burst_t *current_history,
                      int ndm,
                      int ntime,
                      data_t threshold,
                      fifo_data_t *cand
                      );

  void write_cand(
                  fifo_data_t *cand,
		  data_t *out);
  

  void terminate_cand_fifo(
                           fifo_data_t *cand);
  
  void read_history(
                    int ndm,
                    burst_t *history,
                    fifo_burst_t &history_fifo
                    );

  void calculate_cand_wrap(
                           fifo_burst_t &in, 
                           burst_t *previous_history,
                           burst_t *current_history,
                           int ndm,
                           int ntime,
                           data_t threshold,
                           fifo_data_t *cand);
  
  void write_history(
                     int ndm,
                     burst_t *history,
                     fifo_burst_t &history_fifo
                     );
  
  void fifo2history(
                    fifo_burst_t &history_fifo,
                    data_t history[NSAMP_PER_IMG][NBOXCAR-1]
                    );
  
  void history2fifo(
                    data_t history[NSAMP_PER_IMG][NBOXCAR-1],
                    fifo_burst_t &history_fifo
                    );

  void calculate_cand(
                      fifo_burst_t &in, 
                      fifo_burst_t &previous_history_fifo,
                      fifo_burst_t &current_history_fifo,
                      int ndm,
                      int ntime,
                      data_t threshold,
                      fifo_data_t *cand);

  void calculate_cand_worker(
                             int ntime,
                             fifo_burst_t &in,
                             data_t previous_history[NSAMP_PER_IMG][NBOXCAR-1],
                             data_t current_history[NSAMP_PER_IMG][NBOXCAR-1],
                             data_t threshold,
                             fifo_data_t *cand);
  
}

void krnl_box(
              fifo_burst_t &in, 
              burst_t *previous_history,
              burst_t *current_history,
              int ndm,
              int ntime,
              data_t threshold,
              data_t *out
              ){
  const int max_burst_length = BURST_LENGTH;
  
#pragma HLS INTERFACE m_axi port = previous_history offset = slave bundle = gmem0 max_read_burst_length =max_burst_length
#pragma HLS INTERFACE m_axi port = current_history  offset = slave bundle = gmem1 max_read_burst_length =max_burst_length
#pragma HLS INTERFACE m_axi port = out              offset = slave bundle = gmem2 //max_read_burst_length =max_burst_length
#pragma HLS INTERFACE axis port  = in
  
#pragma HLS INTERFACE s_axilite port = previous_history bundle = control
#pragma HLS INTERFACE s_axilite port = current_history  bundle = control
#pragma HLS INTERFACE s_axilite port = ndm              bundle = control
#pragma HLS INTERFACE s_axilite port = ntime            bundle = control
#pragma HLS INTERFACE s_axilite port = threshold        bundle = control  
#pragma HLS INTERFACE s_axilite port = return           bundle = control

#pragma HLS DATA_PACK variable = previous_history
#pragma HLS DATA_PACK variable = current_history

  int i;
  int j;
  int m;
  int n;
  int loc_boxcar;
  int loc_img;

  fifo_data_t cand[NSAMP_PER_BURST];
 loop_setup_cand_fifo:
  for(i = 0; i < NSAMP_PER_BURST; i++){
#pragma HLS PIPELINE
#pragma HLS STREAM variable=cand[i]
  }
  
#pragma HLS DATAFLOW
  fill_cand_fifo(in, previous_history, current_history, ndm, ntime, threshold, cand);
  write_cand(cand, out);
}

void fill_cand_fifo(
                    fifo_burst_t &in, 
                    burst_t *previous_history,
                    burst_t *current_history,
                    int ndm,
                    int ntime,
                    data_t threshold,
                    fifo_data_t *cand){

  calculate_cand_wrap(in, previous_history, current_history, ndm, ntime, threshold, cand);
  terminate_cand_fifo(cand);
}

void write_cand(
                fifo_data_t *cand,
                data_t *out){

  int i;
  int off = 0;
  int nvalid = NSAMP_PER_BURST;
  data_t val;
  const int nsamp_per_burst = NSAMP_PER_BURST;
  
  while(nvalid>0){
    for(i = 0; i < NSAMP_PER_BURST; i++){
#pragma HLS PIPELINE II=nsamp_per_burst
      if(cand[i].read_nb(val)){
        if(val==0){
          nvalid--;
        }
        else{
          out[off] = val;
          off++;
        }
      }
    }
  }  
}

void terminate_cand_fifo(
                         fifo_data_t *cand){
  int n;
  
  // terminate the current block
  for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
    cand[n].write(0);
  }
}


void calculate_cand_wrap(
                         fifo_burst_t &in, 
                         burst_t *previous_history,
                         burst_t *current_history,
                         int ndm,
                         int ntime,
                         data_t threshold,
                         fifo_data_t *cand){
  
  fifo_burst_t previous_history_fifo;
  fifo_burst_t current_history_fifo;
  
#pragma HLS STREAM variable=previous_history_fifo
#pragma HLS STREAM variable=current_history_fifo

#pragma HLS DATAFLOW
  
  read_history(ndm, previous_history, previous_history_fifo);
  calculate_cand(in, previous_history_fifo, current_history_fifo, ndm, ntime, threshold, cand);
  write_history(ndm, current_history, current_history_fifo);
}

void read_history(
                  int ndm,
                  burst_t *history,
                  fifo_burst_t &history_fifo
                  ){
  
  int i;
  int j;
  int m;
  int loc;
  
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024
  loop_read_history:
    for(j = 0; j < (NBOXCAR-1)*NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      loc = i*(NBOXCAR-1)*NBURST_PER_IMG+j;
      history_fifo.write(history[loc]);
    }
  }
}

void write_history(
                   int ndm,
                   burst_t *history,
                   fifo_burst_t &history_fifo
                   ){
  int i;
  int j;
  int m;
  int loc;
  
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024
  loop_read_history:
    for(j = 0; j < (NBOXCAR-1)*NBURST_PER_IMG; j++){
#pragma HLS PIPELINE
      loc = i*(NBOXCAR-1)*NBURST_PER_IMG+j;
      history[loc] = history_fifo.read();
    }
  }
}

void calculate_cand(
                    fifo_burst_t &in, 
                    fifo_burst_t &previous_history_fifo,
                    fifo_burst_t &current_history_fifo,
                    int ndm,
                    int ntime,
                    data_t threshold,
                    fifo_data_t *cand){
  int i;
  data_t history[NSAMP_PER_IMG][NBOXCAR-1];
  
  const int nsamp_per_burst = NSAMP_PER_BURST;
#pragma HLS array_reshape variable=history cyclic factor=nsamp_per_burst dim=1
#pragma HLS array_reshape variable=history complete dim=2
  
  for(i = 0; i < ndm; i++){
    fifo2history(previous_history_fifo, history);
    calculate_cand_worker(ntime, in, history, history, threshold, cand);    
    history2fifo(history, current_history_fifo);
  } 
}

void fifo2history(
                  fifo_burst_t &history_fifo,
                  data_t history[NSAMP_PER_IMG][NBOXCAR-1]
                  ){
  
  int i;
  int j;
  burst_t burst;
  
 loop_fifo2history:
  for(i = 0; i < (NBOXCAR-1)*NBURST_PER_IMG; i++){
#pragma HLS PIPELINE
    burst = history_fifo.read();
    for(j = 0; j < NSAMP_PER_BURST; j++){
      history[(i%NBURST_PER_IMG)*NSAMP_PER_BURST+j][i/NBURST_PER_IMG] = burst.data[j];
    }
  }
}

void calculate_cand_worker(
                           int ntime,
                           fifo_burst_t &in,
                           data_t previous_history[NSAMP_PER_IMG][NBOXCAR-1],
                           data_t current_history[NSAMP_PER_IMG][NBOXCAR-1],
                           data_t threshold,
                           fifo_data_t *cand){
  int j;
  int m;
  int n;
  int k;
  int loc_boxcar;
  int loc_img;
  burst_t burst;
  
  const int nsamp_per_burst = NSAMP_PER_BURST;
  
  data_t boxcar[NBOXCAR];
  data_t max_boxcar[NSAMP_PER_BURST];
  
#pragma HLS array_reshape variable=boxcar complete
#pragma HLS array_reshape variable=max_boxcar complete

  for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
  loop_calculate_cand_worker:
    for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      burst = in.read();
      
      for(n = 0; n < NSAMP_PER_BURST; n++){
        max_boxcar[n] = 0;
        loc_img = m*NSAMP_PER_BURST + n;
	
        // Last boxcar
        boxcar[NBOXCAR-1] = previous_history[loc_img][NBOXCAR-2] + burst.data[n];
        if(boxcar[NBOXCAR-1]>max_boxcar[n]){
          max_boxcar[n]=boxcar[NBOXCAR-1];
        }
        // Boxcars without the first and last one
        for(k = NBOXCAR - 2; k > 0; k--){
          boxcar[k] = previous_history[loc_img][k-1] + burst.data[n];
          if(boxcar[k]>max_boxcar[n]){
            max_boxcar[n]=boxcar[k];
          }
          current_history[loc_img][k] = boxcar[k];
        }	
	
        // First boxcar
        boxcar[0]  = burst.data[n];
        current_history[loc_img][0] = boxcar[0];
        if(boxcar[0]>max_boxcar[n]){
          max_boxcar[n]=boxcar[0];
        }
        
        // Send out cand above threshold
        if(max_boxcar[n]>threshold)
          cand[n].write(max_boxcar[n]);
      }        
    }
  }
}

void history2fifo(
                  data_t history[NSAMP_PER_IMG][NBOXCAR-1],
                  fifo_burst_t &history_fifo
                  ){
  
  int i;
  int j;
  int loc;
  burst_t burst;
  
 loop_history2fifo:
  for(i = 0; i < (NBOXCAR-1)*NBURST_PER_IMG; i++){
#pragma HLS PIPELINE
    for(j = 0; j < NSAMP_PER_BURST; j++){
      loc = i*NSAMP_PER_BURST+j;
      burst.data[j] = history[(i%NBURST_PER_IMG)*NSAMP_PER_BURST+j][i/NBURST_PER_IMG];
    }
    history_fifo.write(burst);
  }
}
