#include "krnl.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {
  void krnl_prep(
                 const burst_real *in1,
                 const burst_real *in2,  
                 const burst_real *cal1,
                 const burst_real *cal2, 
                 const burst_real *sky,
                 burst_real *out,       
                 burst_real *average1,
                 burst_real *average2,
                 int nburst_per_time,
                 int ntime
                 );
  
  void initialize_prepare(
                          int tran,
                          const burst_real *cal1,
                          const burst_real *cal2,
                          const burst_real *sky,
                          real_t *cal1_tile,
                          real_t *cal2_tile,
                          real_t *sky_tile);
  
  void write_average(
                     int tran,
                     real_t *average1_tile,
                     real_t *average2_tile,
                     burst_real *average1,
                     burst_real *average2
                     );

  void read_in(
               int nburst_per_time,
               int ntime,
               const burst_real *in1,
               const burst_real *in2,
               fifo_real &in1_fifo,
               fifo_real &in2_fifo);
  
  void process(
               int nburst_per_time,
               int ntime,
               const burst_real *cal1,
               const burst_real *cal2,
               const burst_real *sky,
               burst_real *average1,
               burst_real *average2,
               fifo_real &in1_fifo,
               fifo_real &in2_fifo,
               fifo_real &out_fifo);
  
  void calculate_average_out(
                             int ntime,
                             real_t *cal1_tile,
                             real_t *cal2_tile,
                             real_t *sky_tile,
                             real_t *average1_tile,
                             real_t *average2_tile,
                             fifo_real &in1_fifo,
                             fifo_real &in2_fifo,
                             fifo_real &out_fifo);
  
  void reset_average(
                     real_t *average1_tile,
                     real_t *average2_tile);
  
  void set_average_out(
                       int ntime,
                       real_t *cal1_tile,
                       real_t *cal2_tile,
                       real_t *sky_tile,
                       real_t *average1_tile,
                       real_t *average2_tile,
                       fifo_real &in1_fifo,
                       fifo_real &in2_fifo,
                       fifo_real &out_fifo);
    
  void write_out(
                 int nburst_per_time,
                 int ntime,
                 fifo_real &out_fifo,
                 burst_real *out);
}

void krnl_prep(
               const burst_real *in1,
               const burst_real *in2,  
               const burst_real *cal1,
               const burst_real *cal2, 
               const burst_real *sky,
               burst_real *out,       
               burst_real *average1,
               burst_real *average2,
               int nburst_per_time,
               int ntime
               )
{
  // Setup the interface, max_*_burst_length defines the max burst length (UG902 for detail)
  const int burst_len = BURST_LEN_PREP;
#pragma HLS INTERFACE m_axi port = in1      offset = slave bundle = gmem0 max_read_burst_length=burst_len  
#pragma HLS INTERFACE m_axi port = in2      offset = slave bundle = gmem1 max_read_burst_length=burst_len  
#pragma HLS INTERFACE m_axi port = cal1     offset = slave bundle = gmem2 max_read_burst_length=burst_len  
#pragma HLS INTERFACE m_axi port = cal2     offset = slave bundle = gmem3 max_read_burst_length=burst_len  
#pragma HLS INTERFACE m_axi port = sky      offset = slave bundle = gmem4 max_read_burst_length=burst_len  
#pragma HLS INTERFACE m_axi port = out      offset = slave bundle = gmem5 max_write_burst_length=burst_len
#pragma HLS INTERFACE m_axi port = average1 offset = slave bundle = gmem6 max_write_burst_length=burst_len
#pragma HLS INTERFACE m_axi port = average2 offset = slave bundle = gmem7 max_write_burst_length=burst_len

#pragma HLS INTERFACE s_axilite port = in1             bundle = control
#pragma HLS INTERFACE s_axilite port = in2             bundle = control
#pragma HLS INTERFACE s_axilite port = cal1            bundle = control
#pragma HLS INTERFACE s_axilite port = cal2            bundle = control
#pragma HLS INTERFACE s_axilite port = sky             bundle = control
#pragma HLS INTERFACE s_axilite port = out             bundle = control
#pragma HLS INTERFACE s_axilite port = average1        bundle = control
#pragma HLS INTERFACE s_axilite port = average2        bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_time bundle = control
#pragma HLS INTERFACE s_axilite port = ntime           bundle = control
    
#pragma HLS INTERFACE s_axilite port = return bundle = control
    
  // Has to use DATA_PACK to enable burst with struct
#pragma HLS DATA_PACK variable = in1
#pragma HLS DATA_PACK variable = in2
#pragma HLS DATA_PACK variable = cal1
#pragma HLS DATA_PACK variable = cal2
#pragma HLS DATA_PACK variable = sky
#pragma HLS DATA_PACK variable = out
#pragma HLS DATA_PACK variable = average1
#pragma HLS DATA_PACK variable = average2

#pragma HLS DATAFLOW
  static fifo_real in1_fifo;
  static fifo_real in2_fifo;
  static fifo_real out_fifo;
#pragma HLS STREAM variable=in1_fifo
#pragma HLS STREAM variable=in2_fifo
#pragma HLS STREAM variable=out_fifo
  
  read_in(
          nburst_per_time,
          ntime,
          in1,
          in2,
          in1_fifo,
          in2_fifo);
  
  process(
          nburst_per_time,
          ntime,
          cal1,
          cal2,
          sky,
          average1,
          average2,
          in1_fifo,
          in2_fifo,
          out_fifo);
  
  write_out(
            nburst_per_time,
            ntime,
            out_fifo,
            out);
}

void read_in(
             int nburst_per_time,
             int ntime,
             const burst_real *in1,
             const burst_real *in2,
             fifo_real &in1_fifo,
             fifo_real &in2_fifo){
  
  int i;
  int j;
  int m;
  int loc;
  const int max_time    = MAX_TIME;
  const int max_tran_per_time  = MAX_CHAN*MAX_BL/TILE_WIDTH_PREP;
  
  int ntran_per_time = nburst_per_time/BURST_LEN_PREP;
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=max_tran_per_time    
    for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT max=max_time
    loop_read_in:
      for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
        loc = j*nburst_per_time + i*BURST_LEN_PREP + m;
        in1_fifo.write(in1[loc]);
        in2_fifo.write(in2[loc]);
      }
    }
  }
}

void process(
             int nburst_per_time,
             int ntime,
             const burst_real *cal1,
             const burst_real *cal2,
             const burst_real *sky,
             burst_real *average1,
             burst_real *average2,
             fifo_real &in1_fifo,
             fifo_real &in2_fifo,
             fifo_real &out_fifo){

  int i;
  int j;
  int m;
  int n;
  burst_real in1_burst;
  burst_real in2_burst;
  burst_real out_burst;
  int loc;
  
  real_t average1_tile[2*TILE_WIDTH_PREP];
  real_t average2_tile[2*TILE_WIDTH_PREP];
  real_t cal1_tile[2*TILE_WIDTH_PREP];	  	      
  real_t cal2_tile[2*TILE_WIDTH_PREP];
  real_t sky_tile[2*TILE_WIDTH_PREP];
  const int ndata_per_burst = 2*NCMPLX_PER_BURST;
#pragma HLS ARRAY_RESHAPE variable=sky_tile  cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal1_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal2_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=average1_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=average2_tile cyclic factor=ndata_per_burst

  //#pragma HLS ARRAY_PARTITION variable=sky_tile  cyclic  factor=ndata_per_burst
  //#pragma HLS ARRAY_PARTITION variable=cal1_tile cyclic factor=ndata_per_burst
  //#pragma HLS ARRAY_PARTITION variable=cal2_tile cyclic factor=ndata_per_burst
  //#pragma HLS ARRAY_PARTITION variable=average1_tile cyclic factor=ndata_per_burst
  //#pragma HLS ARRAY_PARTITION variable=average2_tile cyclic factor=ndata_per_burst
  
  const int max_time          = MAX_TIME;
  const int max_tran_per_time = MAX_CHAN*MAX_BL/TILE_WIDTH_PREP;
  int ntran_per_time          = nburst_per_time/BURST_LEN_PREP;
  
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=max_tran_per_time
#pragma HLS DATAFLOW
    initialize_prepare(i, cal1, cal2, sky, cal1_tile, cal2_tile, sky_tile);
    calculate_average_out(ntime, cal1_tile, cal2_tile, sky_tile, average1_tile, average2_tile, in1_fifo, in2_fifo, out_fifo);
    write_average(i, average1_tile, average2_tile, average1, average2);        
  }  
}

void write_out(
               int nburst_per_time,
               int ntime,
               fifo_real &out_fifo,
               burst_real *out){
  int i;
  int j;
  int m;
  int loc;
  const int max_time           = MAX_TIME;
  const int max_tran_per_time  = MAX_CHAN*MAX_BL/TILE_WIDTH_PREP;
  int ntran_per_time           = nburst_per_time/BURST_LEN_PREP;
  
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=max_tran_per_time
    
  loop_write_out:
    for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT max=max_time
      for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
        loc = j*nburst_per_time + i*BURST_LEN_PREP + m;
        out[loc]=out_fifo.read();
      }
    }
  }
}

void initialize_prepare(
                        int tran,
                        const burst_real *cal1,
                        const burst_real *cal2,
                        const burst_real *sky,
                        real_t *cal1_tile,
                        real_t *cal2_tile,
                        real_t *sky_tile){

  int m;
  int n;
  int loc_burst;
  int loc;
  
 loop_initialize_prepare:
  for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
    loc_burst = tran*BURST_LEN_PREP + m;
    for(n = 0; n < 2*NCMPLX_PER_BURST; n++){
      loc = 2*m*NCMPLX_PER_BURST+n;
      sky_tile[loc]  = sky[loc_burst].data[n];
      cal1_tile[loc] = cal1[loc_burst].data[n];
      cal2_tile[loc] = cal2[loc_burst].data[n];  
    }	  
  }  
}

void write_average(
                   int tran,
                   real_t *average1_tile,
                   real_t *average2_tile,
                   burst_real *average1,
                   burst_real *average2
                   ){
  int m;
  int n;
  int loc;
  int loc_burst;
  burst_real average1_burst;
  burst_real average2_burst;
  
 loop_write_average:
  for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
    for(n = 0; n < 2*NCMPLX_PER_BURST; n++){
      loc = 2*m*NCMPLX_PER_BURST+n;
      average1_burst.data[n] = average1_tile[loc];
      average2_burst.data[n] = average2_tile[loc];
    }
    loc_burst = tran*BURST_LEN_PREP + m;
    average1[loc_burst] = average1_burst;
    average2[loc_burst] = average2_burst;
  }
}

void calculate_average_out(
                           int ntime,
                           real_t *cal1_tile,
                           real_t *cal2_tile,
                           real_t *sky_tile,
                           real_t *average1_tile,
                           real_t *average2_tile,
                           fifo_real &in1_fifo,
                           fifo_real &in2_fifo,
                           fifo_real &out_fifo){
  reset_average(average1_tile, average2_tile);
  set_average_out(ntime, cal1_tile, cal2_tile, sky_tile, average1_tile, average2_tile, in1_fifo, in2_fifo, out_fifo);
}

void reset_average(
                   real_t *average1_tile,
                   real_t *average2_tile){
  int m;
  int n;
  int loc;
  
 loop_reset_average:
  for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
    for(n = 0; n < 2*NCMPLX_PER_BURST; n++){
      loc = 2*m*NCMPLX_PER_BURST+n;
      average1_tile[loc] = 0;
      average2_tile[loc] = 0;    
    }	  
  }
}

void set_average_out(
                     int ntime,
                     real_t *cal1_tile,
                     real_t *cal2_tile,
                     real_t *sky_tile,
                     real_t *average1_tile,
                     real_t *average2_tile,
                     fifo_real &in1_fifo,
                     fifo_real &in2_fifo,
                     fifo_real &out_fifo){
  int j;
  int m;
  int n;
  int loc;
  
  burst_real in1_burst;
  burst_real in2_burst;
  burst_real out_burst;
  const int max_time = MAX_TIME;

  for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT max=max_time
  loop_process:
    for(m = 0; m < BURST_LEN_PREP; m++){
#pragma HLS PIPELINE
      in1_burst = in1_fifo.read();
      in2_burst = in2_fifo.read();
      
      for(n = 0; n < NCMPLX_PER_BURST; n++){
        loc = 2*m*NCMPLX_PER_BURST+2*n;
        
        average1_tile[loc]   += in1_burst.data[2*n];
        average2_tile[loc]   += in2_burst.data[2*n];
        average1_tile[loc+1] += in1_burst.data[2*n+1];
        average2_tile[loc+1] += in2_burst.data[2*n+1];
        
        out_burst.data[2*n] = in1_burst.data[2*n]*cal1_tile[loc] - in1_burst.data[2*n+1]*cal1_tile[loc+1] + 
          in2_burst.data[2*n]*cal2_tile[loc] - in2_burst.data[2*n+1]*cal2_tile[loc+1] - 
          sky_tile[loc];          
        out_burst.data[2*n+1] = in1_burst.data[2*n]*cal1_tile[loc+1] + in1_burst.data[2*n+1]*cal1_tile[loc] + 
          in2_burst.data[2*n]*cal2_tile[loc+1] + in2_burst.data[2*n+1]*cal2_tile[loc] - 
          sky_tile[loc+1];
      }
      out_fifo.write(out_burst);
    }
  }
}
