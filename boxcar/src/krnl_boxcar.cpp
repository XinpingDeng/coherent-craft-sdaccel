#include "boxcar.h"

extern "C"{
  void krnl_boxcar(
                   stream_burst &in,
                   burst_real *history0,
                   burst_real *history1,
                   int ndm,
                   int ntime,
                   real_t threshold,
                   burst_real *out
                   );
  
  void read_history(
                    int nburst_history,
                    burst_real *history,
                    fifo_burst &history_fifo
                    );
  
  void process(
               stream_burst &in, 
               fifo_burst &history0_fifo,
               fifo_burst &history1_fifo,
               int ndm,
               int ntime,
               real_t threshold,
               burst_real *out);
  
  void write_history(
                     int nburst_history,
                     burst_real *history,
                     fifo_burst &history_fifo
                     );

  void fifo2history(
                    fifo_burst &history_fifo,
                    real_t history[NSMP_PER_IMG*(NBOXCAR-1)]
                    );
  
  void history2fifo(
                    real_t history[NSMP_PER_IMG*(NBOXCAR-1)],
                    fifo_burst &history_fifo
                    );

  void process_dm(
                  int ntime,
                  stream_burst &in, 
                  real_t history[NSMP_PER_IMG*(NBOXCAR-1)],
                  real_t threshold,
                  burst_real *out);    
}

void krnl_boxcar(
                 stream_burst &in, 
                 burst_real *history0,
                 burst_real *history1,
                 int ndm,
                 int ntime,
                 real_t threshold,
                 burst_real *out
                 ){
  const int max_burst_length = BURST_LEN_BOXCAR;
  
#pragma HLS INTERFACE m_axi port = history0 offset = slave bundle = gmem0 max_read_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = history1 offset = slave bundle = gmem1 max_read_burst_length=max_burst_length
#pragma HLS INTERFACE m_axi port = out      offset = slave bundle = gmem2 max_read_burst_length=max_burst_length
#pragma HLS INTERFACE axis port  = in
    
#pragma HLS INTERFACE s_axilite port = history0       bundle = control
#pragma HLS INTERFACE s_axilite port = history1       bundle = control
#pragma HLS INTERFACE s_axilite port = ndm            bundle = control
#pragma HLS INTERFACE s_axilite port = ntime          bundle = control
#pragma HLS INTERFACE s_axilite port = threshold      bundle = control  
#pragma HLS INTERFACE s_axilite port = return         bundle = control
  
#pragma HLS DATA_PACK variable = history0
#pragma HLS DATA_PACK variable = history1
#pragma HLS DATA_PACK variable = out

 nburst_history:
  int nburst_history = NBURST_PER_IMG*ndm*(NBOXCAR-1);
  fifo_burst history0_fifo;
  fifo_burst history1_fifo; 
#pragma HLS STREAM variable=history0_fifo
#pragma HLS STREAM variable=history1_fifo 
#pragma HLS DATAFLOW  
  read_history(nburst_history, history0, history0_fifo);
  process(in, history0_fifo, history1_fifo, ndm, ntime, threshold, out);
  write_history(nburst_history, history1, history1_fifo);
}


void read_history(
                  int nburst_history,
                  burst_real *history,
                  fifo_burst &history_fifo
                  ){
  int i;
  const int max_burst_history = MAX_BURST_HISTORY;

 loop_read_history:
  for(i = 0; i < nburst_history; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_burst_history
#pragma HLS PIPELINE
    history_fifo.write(history[i]);
  }
}

void write_history(
                   int nburst_history,
                   burst_real *history,
                   fifo_burst &history_fifo
                   ){
  int i;
  const int max_burst_history = MAX_BURST_HISTORY;

 loop_write_history:
  for(i = 0; i < nburst_history; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_burst_history
#pragma HLS PIPELINE
    history[i] = history_fifo.read();
  }
}

void process(
             stream_burst &in, 
             fifo_burst &history0_fifo,
             fifo_burst &history1_fifo,
             int ndm,
             int ntime,
             real_t threshold,
             burst_real *out){
  int i;
  real_t history[NSMP_PER_IMG*(NBOXCAR-1)];
  
  const int max_dm = MAX_DM;
  const int nsmp_per_process = (NBOXCAR-1)*NREAL_PER_BURST;

#pragma HLS array_reshape   variable=history block factor=nsmp_per_process
  //#pragma HLS array_partition variable=history cyclic factor=nsmp_per_process
    
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT max=max_dm
#pragma HLS DATAFLOW
    fifo2history(history0_fifo, history);
    process_dm(ntime, in, history, threshold, out);    
    history2fifo(history, history1_fifo);
  }
}

void fifo2history(
                  fifo_burst &history_fifo,
                  real_t history[NSMP_PER_IMG*(NBOXCAR-1)]
                  ){  
  int i;
  int j;
  int loc;
  burst_real burst;
  const int max_burst_history_per_dm = MAX_BURST_HISTORY_PER_DM;
  
 loop_fifo2history:
  for(i = 0; i < (NBOXCAR-1)*NBURST_PER_IMG; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max = max_burst_history_per_dm
    burst = history_fifo.read();
    for(j = 0; j < NREAL_PER_BURST; j++){
      loc = i*NREAL_PER_BURST+j;
      history[loc] = burst.data[j];
    }
  }
}

void history2fifo(
                  real_t history[NSMP_PER_IMG*(NBOXCAR-1)],
                  fifo_burst &history_fifo
                  ){  
  int i;
  int j;
  int loc;
  burst_real burst;
  const int max_burst_history_per_dm = MAX_BURST_HISTORY_PER_DM;
  
 loop_history2fifo:
  for(i = 0; i < (NBOXCAR-1)*NBURST_PER_IMG; i++){
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max = max_burst_history_per_dm
    for(j = 0; j < NREAL_PER_BURST; j++){
      loc = i*NREAL_PER_BURST+j;
      burst.data[j] = history[loc];
    }
    history_fifo.write(burst);
  }
}

void process_dm(
                int ntime,
                stream_burst &in, 
                real_t history[NSMP_PER_IMG*(NBOXCAR-1)],
                real_t threshold,
                burst_real *out){
}
