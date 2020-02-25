#include "boxcar.h"

extern "C"{
  void knl_boxcar_threshold_cand(
				 hls::stream<burst_t> &in,
				 burst_t *previous_history,
				 burst_t *current_history,
				 int ndm,
				 int ntime,
				 core_t threshold,
				 core_t *out
				 );
  
  void boxcar_threshold_cand(
			     hls::stream<burst_t> &in, 
			     burst_t *previous_history,
			     burst_t *current_history,
			     int ndm,
			     int ntime,
			     core_t threshold,
			     hls::stream<core_t> cand[NSAMP_PER_BURST]
			     );
  
  void write_cand(hls::stream<core_t> cand[NSAMP_PER_BURST],
		  core_t *out);
}

void knl_boxcar_threshold_cand(
			       hls::stream<burst_t> &in, 
			       burst_t *previous_history,
			       burst_t *current_history,
			       int ndm,
			       int ntime,
			       core_t threshold,
			       core_t *out
			       ){
  const int max_burst_length = MAX_BURST_LENGTH;
  
#pragma HLS INTERFACE m_axi port = previous_history offset = slave bundle = gmem0 max_read_burst_length =max_burst_length
#pragma HLS INTERFACE m_axi port = current_history  offset = slave bundle = gmem1 max_read_burst_length =max_burst_length
#pragma HLS INTERFACE m_axi port = out              offset = slave bundle = gmem2 max_read_burst_length =max_burst_length
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

  hls::stream<core_t> cand[NSAMP_PER_BURST];
 LOOP_SETUP_STREAM:
  for(i = 0; i < NSAMP_PER_BURST; i++){
#pragma HLS PIPELINE
#pragma HLS STREAM variable=cand[i]
  }
  
#pragma HLS DATAFLOW
  boxcar_threshold_cand(in, previous_history, current_history, ndm, ntime, threshold, cand);
  write_cand(cand, out);
}

void boxcar_threshold_cand(
			   hls::stream<burst_t> &in, 
			   burst_t *previous_history,
			   burst_t *current_history,
			   int ndm,
			   int ntime,
			   core_t threshold,
			   hls::stream<core_t> cand[NSAMP_PER_BURST]){
  int i;
  int j;
  int m;
  int n;
  int k;
  int loc_boxcar;
  int loc_img;
  burst_t in_tmp;

#pragma HLS DATA_PACK variable=previous_history
#pragma HLS DATA_PACK variable=current_history
  
  const int nsamp_per_burst  = NSAMP_PER_BURST;
  const int nboxcar = NBOXCAR;

  core_t boxcar[NBOXCAR];
  core_t max_boxcar[NSAMP_PER_BURST];
  core_t history[NSAMP_PER_IMG][NBOXCAR-1];

#pragma HLS array_partition variable=boxcar complete
#pragma HLS array_partition variable=max_boxcar complete
#pragma HLS array_partition variable=history cyclic factor=nsamp_per_burst dim=1
  
#pragma HLS DEPENDENCE variable=history intra false
#pragma HLS DEPENDENCE variable=history inter false
  
 LOOP_NDM:
  for(i = 0; i < ndm; i++){
#pragma HLS LOOP_TRIPCOUNT min=1024 max=1024

  LOOP_READ_HISTORY:
    for(m = 0; m < (NBOXCAR-1)*NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	loc_boxcar = m/NBURST_PER_IMG;
	loc_img    = (m%NBURST_PER_IMG)*NSAMP_PER_BURST + n;
	
	//history[loc_img][loc_boxcar] = previous_history[m].data[n];
      }
    }
    
    for(j = 0; j < ntime; j++){
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    LOOP_WORK_PER_TIME:
      for(m = 0; m < NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
	in_tmp = in.read();
	
	for(n = 0; n < NSAMP_PER_BURST; n++){
	  max_boxcar[n] = 0;
	  loc_img = m*NSAMP_PER_BURST + n;
	  
	  // Last boxcar
	  //boxcar[NBOXCAR-1] = history[loc_img][NBOXCAR-2] + in_tmp.data[n];
	  if(boxcar>max_boxcar[n]){
	    max_boxcar[n]=boxcar[NBOXCAR-1];
	  }
	  // Boxcars without the first and last one
	  for(k = NBOXCAR - 2; k > 0; k--){
	    boxcar[k] = history[loc_img][k-1] + in_tmp.data[n];
	    if(boxcar>max_boxcar[n]){
	      max_boxcar[n]=boxcar[k];
	    }
	  }	
	  //for(k = NBOXCAR - 2; k > 0; k--){
	  //  history[loc_img][k] = boxcar[k];
	  //}
	  
	  // First boxcar
	  boxcar[0] = in_tmp.data[n];
	  //history[loc_img][0] = boxcar[0];
	  if(boxcar>max_boxcar[n]){
	    max_boxcar[n]=boxcar[0];
	  }
	  
	  // Send out cand above threshold
	  if(max_boxcar[n]>threshold)
	    cand[n].write(max_boxcar[n]);
	} 	
      }
    }
    
  LOOP_WRITE_HISTORY:
    for(m = 0; m < (NBOXCAR-1)*NBURST_PER_IMG; m++){
#pragma HLS PIPELINE
      for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
	loc_boxcar = m/NBURST_PER_IMG;
	loc_img    = (m%NBURST_PER_IMG)*NSAMP_PER_BURST + n;
	//current_history[m].data[n] = history[loc_img][loc_boxcar];
      }
    }    
  }

  // Finish the current block
  for(n = 0; n < NSAMP_PER_BURST; n++){
#pragma HLS UNROLL
    cand[n].write(0);
  }
}


void write_cand(hls::stream<core_t> cand[NSAMP_PER_BURST],
		core_t *out){

  int i;
  int off = 0;
  int nvalid = NSAMP_PER_BURST;
  core_t val;
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
