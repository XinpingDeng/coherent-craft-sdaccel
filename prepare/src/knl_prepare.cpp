#include "prepare.h"

// Order is assumed to be TBFP, BFP or BF
extern "C" {
  void knl_prepare(
		   const burst_t *in1,
		   const burst_t *in2,  
		   const burst_t *cal1,
		   const burst_t *cal2, 
		   const burst_t *sky,
		   burst_t *out,       
		   burst_t *average1,
		   burst_t *average2,
		   int nburst_per_time,
		   int ntime_per_cu
		   );
  
  void initialize_prepare(
                        int tran,
                        const burst_t *cal1,
                        const burst_t *cal2,
                        const burst_t *sky,
                        data_t *cal1_tile,
                        data_t *cal2_tile,
                        data_t *sky_tile,
                        data_t *average1_tile,
                        data_t *average2_tile);
  
  void write_average(
                     int tran,
                     data_t *average1_tile,
                     data_t *average2_tile,
                     burst_t *average1,
                     burst_t *average2
                     );

  void read_in(
               int nburst_per_time,
               int ntime_per_cu,
               const burst_t *in1,
               const burst_t *in2,
               fifo_t &in_fifo1,
               fifo_t &in_fifo2);
  
  void process(
               int nburst_per_time,
               int ntime_per_cu,
               const burst_t *cal1,
               const burst_t *cal2,
               const burst_t *sky,
               burst_t *average1,
               burst_t *average2,
               fifo_t &in_fifo1,
               fifo_t &in_fifo2,
               fifo_t &out_fifo);
  
  void calculate_average_out(
                             int ntime_per_cu,
                             data_t *cal1_tile,
                             data_t *cal2_tile,
                             data_t *sky_tile,
                             data_t *average1_tile,
                             data_t *average2_tile,
                             fifo_t &in_fifo1,
                             fifo_t &in_fifo2,
                             fifo_t &out_fifo);
  
  void write_out(
                 int nburst_per_time,
                 int ntime_per_cu,
                 fifo_t &out_fifo,
                 burst_t *out);
}

void knl_prepare(
		 const burst_t *in1,
		 const burst_t *in2,  
		 const burst_t *cal1,
		 const burst_t *cal2, 
		 const burst_t *sky,
		 burst_t *out,       
		 burst_t *average1,
		 burst_t *average2,
		 int nburst_per_time,
		 int ntime_per_cu
		 )
{
  // Setup the interface, max_*_burst_length defines the max burst length (UG902 for detail)
#pragma HLS INTERFACE m_axi port = in1      offset = slave bundle = gmem0 max_read_burst_length=64  
#pragma HLS INTERFACE m_axi port = in2      offset = slave bundle = gmem1 max_read_burst_length=64  
#pragma HLS INTERFACE m_axi port = cal1     offset = slave bundle = gmem2 max_read_burst_length=64  
#pragma HLS INTERFACE m_axi port = cal2     offset = slave bundle = gmem3 max_read_burst_length=64  
#pragma HLS INTERFACE m_axi port = sky      offset = slave bundle = gmem4 max_read_burst_length=64  
#pragma HLS INTERFACE m_axi port = out      offset = slave bundle = gmem5 max_write_burst_length=64
#pragma HLS INTERFACE m_axi port = average1 offset = slave bundle = gmem6 max_write_burst_length=64
#pragma HLS INTERFACE m_axi port = average2 offset = slave bundle = gmem7 max_write_burst_length=64

#pragma HLS INTERFACE s_axilite port = in1         bundle = control
#pragma HLS INTERFACE s_axilite port = in2         bundle = control
#pragma HLS INTERFACE s_axilite port = cal1        bundle = control
#pragma HLS INTERFACE s_axilite port = cal2        bundle = control
#pragma HLS INTERFACE s_axilite port = sky         bundle = control
#pragma HLS INTERFACE s_axilite port = out         bundle = control
#pragma HLS INTERFACE s_axilite port = average1    bundle = control
#pragma HLS INTERFACE s_axilite port = average2    bundle = control
#pragma HLS INTERFACE s_axilite port = nburst_per_time bundle = control
#pragma HLS INTERFACE s_axilite port = ntime_per_cu    bundle = control
    
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
  static fifo_t in_fifo1;
  static fifo_t in_fifo2;
  static fifo_t out_fifo;
#pragma HLS STREAM variable=in_fifo1
#pragma HLS STREAM variable=in_fifo2
#pragma HLS STREAM variable=out_fifo
  
  read_in(
          nburst_per_time,
          ntime_per_cu,
          in1,
          in2,
          in_fifo1,
          in_fifo2);
  
  process(
          nburst_per_time,
          ntime_per_cu,
          cal1,
          cal2,
          sky,
          average1,
          average2,
          in_fifo1,
          in_fifo2,
          out_fifo);
  
  write_out(
            nburst_per_time,
            ntime_per_cu,
            out_fifo,
            out);
}

void read_in(
             int nburst_per_time,
             int ntime_per_cu,
             const burst_t *in1,
             const burst_t *in2,
             fifo_t &in_fifo1,
             fifo_t &in_fifo2){
  
  int i;
  int j;
  int m;
  int loc;
  const int mtime_per_cu    = MTIME_PER_CU;
  const int mtran_per_time  = MCHAN*MBASELINE/TILE_WIDTH;
  
  int ntran_per_time = nburst_per_time/BURST_LENGTH;
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=mtran_per_time    
    for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT max=mtime_per_cu
    loop_read_in:
      for(m = 0; m < BURST_LENGTH; m++){
#pragma HLS PIPELINE
        loc = j*nburst_per_time + i*BURST_LENGTH + m;
        in_fifo1.write(in1[loc]);
        in_fifo2.write(in2[loc]);
      }
    }
  }
}

void process(
             int nburst_per_time,
             int ntime_per_cu,
             const burst_t *cal1,
             const burst_t *cal2,
             const burst_t *sky,
             burst_t *average1,
             burst_t *average2,
             fifo_t &in_fifo1,
             fifo_t &in_fifo2,
             fifo_t &out_fifo){

  int i;
  int j;
  int m;
  int n;
  burst_t in1_burst;
  burst_t in2_burst;
  burst_t out_burst;
  int loc;
  
  data_t average1_tile[2*TILE_WIDTH];
  data_t average2_tile[2*TILE_WIDTH];
  data_t cal1_tile[2*TILE_WIDTH];	  	      
  data_t cal2_tile[2*TILE_WIDTH];
  data_t sky_tile[2*TILE_WIDTH];
  const int ndata_per_burst = 2*NSAMP_PER_BURST;
#pragma HLS ARRAY_RESHAPE variable=sky_tile  cyclic  factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal1_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=cal2_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=average1_tile cyclic factor=ndata_per_burst
#pragma HLS ARRAY_RESHAPE variable=average2_tile cyclic factor=ndata_per_burst
  
  const int mtime_per_cu    = MTIME_PER_CU;
  const int mtran_per_time  = MCHAN*MBASELINE/TILE_WIDTH;
  int ntran_per_time = nburst_per_time/BURST_LENGTH;
  
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=mtran_per_time
    initialize_prepare(i, cal1, cal2, sky, cal1_tile, cal2_tile, sky_tile, average1_tile, average2_tile);
    calculate_average_out(ntime_per_cu, cal1_tile, cal2_tile, sky_tile, average1_tile, average2_tile, in_fifo1, in_fifo2, out_fifo);
    write_average(i, average1_tile, average2_tile, average1, average2);        
  }  
}

void write_out(
               int nburst_per_time,
               int ntime_per_cu,
               fifo_t &out_fifo,
               burst_t *out){
  int i;
  int j;
  int m;
  int loc;
  const int mtime_per_cu    = MTIME_PER_CU;
  const int mtran_per_time  = MCHAN*MBASELINE/TILE_WIDTH;
  int ntran_per_time = nburst_per_time/BURST_LENGTH;
  
  for(i = 0; i < ntran_per_time; i++){
#pragma HLS LOOP_TRIPCOUNT  max=mtran_per_time
    
  loop_write_out:
    for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT max=mtime_per_cu
      for(m = 0; m < BURST_LENGTH; m++){
#pragma HLS PIPELINE
        loc = j*nburst_per_time + i*BURST_LENGTH + m;
        out[loc]=out_fifo.read();
      }
    }
  }
}

void initialize_prepare(
                        int tran,
                        const burst_t *cal1,
                        const burst_t *cal2,
                        const burst_t *sky,
                        data_t *cal1_tile,
                        data_t *cal2_tile,
                        data_t *sky_tile,
                        data_t *average1_tile,
                        data_t *average2_tile){

  int m;
  int n;
  int loc_burst;
  int loc;
  
 loop_initialize_prepare:
  for(m = 0; m < BURST_LENGTH; m++){
#pragma HLS PIPELINE
    loc_burst = tran*BURST_LENGTH + m;
    for(n = 0; n < 2*NSAMP_PER_BURST; n++){
      loc = 2*m*NSAMP_PER_BURST+n;
      sky_tile[loc]  = sky[loc_burst].data[n];
      cal1_tile[loc] = cal1[loc_burst].data[n];
      cal2_tile[loc] = cal2[loc_burst].data[n];  
      average1_tile[loc] = 0;
      average2_tile[loc] = 0;    
    }	  
  }  
}

void write_average(
                   int tran,
                   data_t *average1_tile,
                   data_t *average2_tile,
                   burst_t *average1,
                   burst_t *average2
                   ){
  int m;
  int n;
  int loc;
  int loc_burst;
  burst_t average1_burst;
  burst_t average2_burst;
  
 loop_write_average:
  for(m = 0; m < BURST_LENGTH; m++){
#pragma HLS PIPELINE
    for(n = 0; n < 2*NSAMP_PER_BURST; n++){
      loc = 2*m*NSAMP_PER_BURST+n;
      average1_burst.data[n] = average1_tile[loc];
      average2_burst.data[n] = average1_tile[loc];
    }
    loc_burst = tran*BURST_LENGTH + m;
    average1[loc_burst] = average1_burst;
    average2[loc_burst] = average2_burst;
  }
}

void calculate_average_out(
                           int ntime_per_cu,
                           data_t *cal1_tile,
                           data_t *cal2_tile,
                           data_t *sky_tile,
                           data_t *average1_tile,
                           data_t *average2_tile,
                           fifo_t &in_fifo1,
                           fifo_t &in_fifo2,
                           fifo_t &out_fifo){
  int j;
  int m;
  int n;
  int loc;
  
  burst_t in1_burst;
  burst_t in2_burst;
  burst_t out_burst;
  const int mtime_per_cu    = MTIME_PER_CU;
  
  for(j = 0; j < ntime_per_cu; j++){
#pragma HLS LOOP_TRIPCOUNT max=mtime_per_cu
  loop_process:
    for(m = 0; m < BURST_LENGTH; m++){
#pragma HLS PIPELINE
      in1_burst = in_fifo1.read();
      in2_burst = in_fifo2.read();
        
      for(n = 0; n < 2*NSAMP_PER_BURST; n++){
        loc = 2*m*NSAMP_PER_BURST+n;
        average1_tile[loc] += in1_burst.data[n];
        average2_tile[loc] += in2_burst.data[n];
      }  
      
      for(n = 0; n < NSAMP_PER_BURST; n++){
        loc = 2*m*NSAMP_PER_BURST+2*n;
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
