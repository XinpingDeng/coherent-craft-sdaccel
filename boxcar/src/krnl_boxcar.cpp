#include "boxcar.h"

extern "C"{
  void krnl_boxcar(
                   stream_burst &in,                        
                   int ndm,
                   int ntime,
                   int nburst_per_img,
                   burst_real *history_in,
                   burst_real *history_out,
                   cand_krnl_t *out
                   );
  
  void read_history(                    
                    int ndm,
                    int nburst_per_img,
                    burst_real *history,
                    fifo_burst &history_fifo
                                        );

  void fifo2history(
                    int nburst_per_img,
                    fifo_burst &history_fifo,
                    burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1]
                    );

  void history2fifo(
                    int nburst_per_img,
                    fifo_burst &history_fifo,
                    burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1]
                    );
  
  void process(                                     
               int ndm,
               int ntime,
               int nburst_per_img,
               stream_burst &in, 
               fifo_burst &history_in_fifo,
               fifo_burst &history_out_fifo,
               cand_krnl_t *out
                                                    );
  
  void write_history(
                     int ndm,
                     int nburst_per_img,
                     burst_real *history,
                     fifo_burst &history_fifo
                     );
  
  void boxcar2cand(                                                        
                   int ndm,
                   int ntime,
                   int nburst_per_img,
                   stream_burst &in,
                   fifo_burst &history_in_fifo,
                   fifo_burst &history_out_fifo,
                   fifo_cand cand_fifo[NREAL_PER_BURST]
                                                                           );
  
  void write_cand(
                  cand_krnl_t *out,
                  fifo_cand cand_fifo[NREAL_PER_BURST]
                  );
  
  void boxcar2cand_dm(
                      int dm,                         
                      int ntime,
                      int nburst_per_img,
                      stream_burst &in,
                      accum_t scale_factor[NBOXCAR],
                      burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1],
                      fifo_cand cand_fifo[NREAL_PER_BURST]
                      );

  void boxcar2cand_time(
                        int dm,
                        int time, 
                        int nburst_per_img,
                        stream_burst &in,
                        accum_t scale_factor[NBOXCAR],
                        burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1],
                        fifo_cand cand_fifo[NREAL_PER_BURST]
                        );
}

// This is the top level function
// reads data from memory 512 bits wide, computes boxcar and writes out candidates
// above the S/N threshold.
void krnl_boxcar(
                 stream_burst &in,                     
                 int ndm,
                 int ntime,
                 int nburst_per_img,
                 burst_real *history_in,
                 burst_real *history_out,
                 cand_krnl_t *out){
  // Output of Alex's FFT will be an hls::stream  512 bits wide I think.
  // Which will send 32 numbers per clock
  // Rastering throught the output image for a single image until it gets to the next image time
  // Then the next DM.
  
#pragma HLS INTERFACE axis      port = in
#pragma HLS INTERFACE m_axi     port = history_in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi     port = history_out offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi     port = out      offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = ndm            bundle = control
#pragma HLS INTERFACE s_axilite port = ntime          bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_img bundle = control
#pragma HLS INTERFACE s_axilite port = history_in       bundle = control
#pragma HLS INTERFACE s_axilite port = history_out       bundle = control
#pragma HLS INTERFACE s_axilite port = out            bundle = control
#pragma HLS INTERFACE s_axilite port = return         bundle = control
  
  fifo_burst history_in_fifo;
  fifo_burst history_out_fifo;
#pragma HLS DATAFLOW
  
  read_history(ndm, nburst_per_img, history_in, history_in_fifo);
  process(ndm, ntime, nburst_per_img, in, history_in_fifo, history_out_fifo, out);
  write_history(ndm, nburst_per_img, history_out, history_out_fifo);
}

void read_history(
                  int ndm,
                  int nburst_per_img,
                  burst_real *history,
                  fifo_burst &history_fifo
                  ){
  int i;
  const int max_loop_i = MAX_DM*MAX_BURST_PER_IMG*(NBOXCAR-1);
  
 loop_read_history:
  for(i = 0; i < ndm*nburst_per_img*(NBOXCAR-1); i++){
#pragma HLS LOOP_TRIPCOUNT max=max_loop_i
#pragma HLS PIPELINE
    history_fifo.write(history[i]);
  }
}

void process(
             int ndm,
             int ntime,
             int nburst_per_img,
             stream_burst &in, 
             fifo_burst &history_in_fifo,
             fifo_burst &history_out_fifo,
             cand_krnl_t *out){
  
  fifo_cand cand_fifo[NREAL_PER_BURST];
#pragma HLS DATAFLOW
  boxcar2cand(ndm, ntime, nburst_per_img, in, history_in_fifo, history_out_fifo, cand_fifo);
  write_cand(out, cand_fifo);
}

void write_history(
                   int ndm,
                   int nburst_per_img,
                   burst_real *history,
                   fifo_burst &history_fifo
                   ){
  int i;
  const int max_loop_i = MAX_DM*MAX_BURST_PER_IMG*(NBOXCAR-1);
  
 loop_write_history:
  for(i = 0; i < ndm*nburst_per_img*(NBOXCAR-1); i++){
#pragma HLS LOOP_TRIPCOUNT max=max_loop_i
#pragma HLS PIPELINE
    history[i] = history_fifo.read();
  }
}

void boxcar2cand(
                 int ndm,
                 int ntime,
                 int nburst_per_img,
                 stream_burst &in,
                 fifo_burst &history_in_fifo,
                 fifo_burst &history_out_fifo,
                 fifo_cand cand_fifo[NREAL_PER_BURST]){

  int i;
  int dm;
  burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1];
  accum_t scale_factor[NBOXCAR];

  const int max_loop_dm = MAX_DM;

  //#pragma HLS ARRAY_PARTITION variable=history dim=2 complete
#pragma HLS ARRAY_RESHAPE variable=history dim=2 complete
  
  // Setup scale factor
#pragma HLS ARRAY_RESHAPE variable=scale_factor
  for(i = 0; i < NBOXCAR; i++){
#pragma HLS UNROLL
    scale_factor[i] = hls::rsqrtf(i+1);
  }
  
 loop_boxcar2cand:
  for(dm = 0; dm < ndm; dm++){
#pragma HLS LOOP_TRIPCOUNT max=max_loop_dm
    fifo2history(nburst_per_img, history_in_fifo, history);
    boxcar2cand_dm(dm, ntime, nburst_per_img, in, scale_factor, history, cand_fifo);
    history2fifo(nburst_per_img, history_out_fifo, history);
  }
  
  // write termination to each fifo so the next process will quit
  cand_krnl_t finish_cand;
  finish_cand(0,15) = -1;
 loop_terminate:
  for(i = 0; i < NREAL_PER_BURST; i++){
#pragma HLS PIPELINE
    cand_fifo[i].write(finish_cand); // finishc_cand is the termination marker
  }
}

void write_cand(
                cand_krnl_t *out,
                fifo_cand cand_fifo[NREAL_PER_BURST]){
  
  int i;
  int nvalid = NREAL_PER_BURST;
  int ncand  = 0;
  
  cand_krnl_t cand;
  
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
                    int dm,
                    int ntime,
                    int nburst_per_img,
                    stream_burst &in,
                    accum_t scale_factor[NBOXCAR],
                    burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1],
                    fifo_cand cand_fifo[NREAL_PER_BURST]){
  int time;
  const int max_loop_time = MAX_TIME;
  
  for(time = 0; time < ntime; time++){
#pragma HLS LOOP_TRIPCOUNT max=max_loop_time
    boxcar2cand_time(dm, time, nburst_per_img, in, scale_factor, history, cand_fifo);
  }
}

void boxcar2cand_time(
                      int dm,
                      int time,
                      int nburst_per_img,
                      stream_burst &in,
                      accum_t scale_factor[NBOXCAR],
                      burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1],
                      fifo_cand cand_fifo[NREAL_PER_BURST]){
  int i;
  int j;
  int k;
  int loc_img;
  cand_krnl_t cand;
  accum_t boxcar;
  accum_t top_boxcar;
  accum_t scaled_boxcar;
  stream_burst_core burst;
  const int max_loop_i = MAX_BURST_PER_IMG;
  
  burst_real tile[NBOXCAR];
#pragma HLS ARRAY_RESHAPE variable=tile
  
  for(i = 0; i < nburst_per_img; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_loop_i
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
          cand(40, 47) = time;
          cand(48, 63) = dm;
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
                  int nburst_per_img,
                  fifo_burst &history_fifo,
                  burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1]
                  ){
  int i;
  int j;
  const int max_loop_i = MAX_BURST_PER_IMG;
  
  for(i = 0; i < nburst_per_img; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_loop_i
  loop_fifo2history:
    for(j = 0; j < NBOXCAR-1; j++){
#pragma HLS PIPELINE
      history[i][j] = history_fifo.read();
    }
  }
}

void history2fifo(
                  int nburst_per_img,
                  fifo_burst &history_fifo,
                  burst_real history[MAX_BURST_PER_IMG][NBOXCAR-1]
                  ){
  int i;
  int j;
  const int max_loop_i = MAX_BURST_PER_IMG;
    
  for(i = 0; i < nburst_per_img; i++){
#pragma HLS LOOP_TRIPCOUNT max = max_loop_i
  loop_fifo2history:
    for(j = 0; j < NBOXCAR-1; j++){
#pragma HLS PIPELINE
      history_fifo.write(history[i][j]);
    }
  }
}
