#ifndef _BOXCAR_H
#define _BOXCAR_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <ap_fixed.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define BURST_WIDTH      512
#define REAL_WIDTH       16
#define MAX_CAND         1024*16 // Maximum number of candidates to generate
#define INT_WIDTH_BOXCAR 5
#define INT_WIDTH_ACCUM  10

#define NPIX            256 // Number of pixels on a side of the image
#define NBOXCAR         16  // Number of boxcars to create
#define NTIME           256 // Number of time samples to process
#define NDM             1024 // Number of DMs to search

//#define NPIX            8 // Number of pixels on a side of the image
//#define NBOXCAR         8 // Number of boxcars to create
//#define NTIME           8 // Number of time samples to process
//#define NDM             4 // Number of DMs to search

#define NREAL_PER_BURST (BURST_WIDTH/REAL_WIDTH) // Number of PIXELS to process at a time
#define NPIX2           (NPIX*NPIX) // Number of pixels in the (square) image
#define NBURST_PER_IMG  (NPIX2/NREAL_PER_BURST) // Number of processing block per image

// For boxcar samples
typedef ap_fixed<REAL_WIDTH, INT_WIDTH_BOXCAR, AP_RND, AP_SAT> real_t;
// For accumulated boxcar samples
typedef ap_fixed<REAL_WIDTH, INT_WIDTH_ACCUM,  AP_RND, AP_SAT> accum_t;

// For top-level STREAM
typedef ap_axiu<BURST_WIDTH, 0, 0, 0>  stream_burst_core;
typedef hls::stream<stream_burst_core> stream_burst;

typedef struct cand_t {
  accum_t  snr;          // S/N
  uint16_t loc_img;      // pixel index
  uint8_t  boxcar_width; // boxcar
  
  uint8_t  t;        // sample number where max S/N occurs
  uint16_t dm_index; // DM index
}cand_t;

const real_t THRESHOLD   = real_t(10); 
const cand_t FINISH_CAND = {accum_t(-1), uint16_t(0), uint8_t(0), uint8_t(0), uint16_t(0)};

typedef struct burst_real{
  real_t data[NREAL_PER_BURST];
}burst_real;

typedef hls::stream<burst_real> fifo_burst;
typedef hls::stream<cand_t>     fifo_cand;

#endif
