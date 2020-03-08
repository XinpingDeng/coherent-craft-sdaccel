#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>
#include <assert.h>
#include <math.h>

#include "boxcar.h"


static int ncand;


void read_bcblock2(const int idm, int t, const boxcar_block* pin, hls::stream<boxcar_block>& in) {

  for(int p = 0; p < BLOCK_PER_IMAGE; p++) {
#pragma HLS PIPELINE II=1
    in << pin[p + BLOCK_PER_IMAGE*(t + NT*idm)];
  }
}

void doboxcar_and_thresh_bcblock(hls::stream<boxcar_block>& in, hls::stream<thresh_cand_t> out[SAMP_PER_BLOCK]) {

  static boxcar_t history[NPIX2][NBOX] = {boxcar_t(0)};
  // needs to partition complete to turn it into shift registers
#pragma HLS array_partition variable=history dim=1 factor=SAMP_PER_BLOCK cyclic

  //#pragma HLS array_reshape variable=history dim=0 complete
  //#pragma HLS resource variable=history core=XPM_MEMORY uram //- changing to URAM makes it go to II=16.

 block_loop:
  for (int p  = 0; p < BLOCK_PER_IMAGE; p++) {
#pragma HLS PIPELINE II=1
    boxcar_block bin;
#pragma HLS DATA_PACK variable=bin

    in >> bin;
  samploop:
    for (int x = 0; x < SAMP_PER_BLOCK; x++) {
      int idx = p*SAMP_PER_BLOCK + x;

      // Shift data along in the FIFO
    shiftloop:
      for (int w = NBOX-1; w >0; w--) {
        history[idx][w] = history[idx][w-1];
      }
      // put new data in the FIFO
      history[idx][0] = bin.data[x];

      // set initial value
      thresh_cand_t tcand;
      tcand.sn = 0;
      tcand.width = 0;
      tcand.x = idx;
      boxcar_accum_t b = 0;

    widthloop:
      for (int w = 0; w < NBOX; w++) { // Is b a carried dependance?
#pragma HLS unroll
        b += history[idx][w];
        const boxcar_accum_t correction(1.0f/sqrtf(w+1));

        // The following multiplication results in a timing violation
        boxcar_accum_t bthresh = b*correction;

        // Adding the following HLS resource latency results in "SYN 201-103" - Cannot apply unit assignment due to
        // incompatible operation sets. It' strying to do 'shl' and 'add'
#pragma HLS RESOURCE variable=bthresh latency=2 core=AddSubnS

        if (bthresh > tcand.sn) {
          tcand.sn = bthresh;
          tcand.width = w+1;
        }

      }

      if (tcand.sn >= THRESHOLD) {
        if(out[x].write_nb(tcand)) {
          // succesful write
        } else {
          // write failed - TODO increment a counter as a status thing.
        }
      }
    }
  }
  // write termination to each fifo so the next process will quit
  for (int x = 0; x < SAMP_PER_BLOCK; x++) {
    out[x].write(FINISH_THRESH_CAND); // FINISH_THRE is the termination marker
  }
}


int write_cand(const int idm,const int t, cand_t* pout, hls::stream<thresh_cand_t> out[SAMP_PER_BLOCK]) {

  int nvalid = SAMP_PER_BLOCK;
  //	static int ncand;
  //	if (idm == 0 && t == 0) {
  //		ncand = 0;
  //	}
  assert(ncand < MAX_CAND);
  while(nvalid > 0) {
    for (int x = 0; x < SAMP_PER_BLOCK; x++) {
      thresh_cand_t val;

      if(out[x].read_nb(val)) {
        // successfull
        if (val.sn == FINISH_THRESH_CAND.sn &&
            val.width == FINISH_THRESH_CAND.width &&
            val.x == FINISH_THRESH_CAND.x) { // finished signal
          nvalid--;
        } else {
          cand_t cand;
          cand.idm = idm;
          cand.t = t;
          cand.sn = val.sn;
          cand.width = val.width;
          cand.x = val.x;
          pout[ncand] = cand;
          ncand++;
          if (ncand >= MAX_CAND-1) { // avoid overwiting the end of the array
            nvalid = 0; // sends signal to quit.
          }
        }
      } else {
        // failed - fifo didn't contain anything so don't really care
      }
    }
  }

  if (t == NT -1 && idm == NDM - 1) {
    cand_t end;
    end.idm = -1;
    end.t = -1;
    end.sn = -1;
    end.width = -1;
    end.x = -1;
    pout[ncand] = end;
  }

  return ncand;

}

// This is the top level function
// reads data from memory 512 bits wide, computes boxcar and writes out candidates
// above the S/N threshold.
int boxcar_and_thresh(boxcar_block* pin, cand_t* pout) {

  // Output of Alex's FFT will be an hls::stream  512 bits wide I think.
  // Which will send 32 numbers per clock
  // Rastering throught the output image for a single image until it gets to the next image time
  // Then the next DM.
#pragma HLS interface m_axi port=pin depth=NPIX*NPIX*NDM*NT/SAMP_PER_BLOCK offset=slave bundle=gmem0
#pragma HLS interface m_axi port=pout depth=NPIX*NPIX*NBOX*NDM*NT/SAMP_PER_BLOCK offset=slave bundle=gmem1
#pragma HLS interface s_axilite port=return bundle=control
#pragma HLS DATA_PACK variable=pin

  ncand = 0;
 main_dmloop:
  for (int idm = 0; idm < NDM; idm++) {

  main_tloop:
    for (int t = 0; t < NT; t++) {
      // THisis one of the canonical forms for dataflow
#pragma HLS loop_tripcount min=NDM*NT max=NDM*NT avg=NDM*NT
#pragma HLS dataflow
      hls::stream<boxcar_block> in;
      hls::stream<thresh_cand_t> out[SAMP_PER_BLOCK];

      //printf("idm=%d t=%d\n", idm, t);
      read_bcblock2(idm, t, pin, in);
      doboxcar_and_thresh_bcblock(in, out);
      write_cand(idm, t, pout, out);
    }
  }

  return ncand;
}

