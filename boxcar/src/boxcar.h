#ifndef _REAL_H
#define _REAL_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <ap_fixed.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_math.h>

#define MAX_PALTFORMS    16
#define MAX_DEVICES      16
#define PARAM_VALUE_SIZE 1024
#define MEM_ALIGNMENT    4096  // memory alignment on device
#define LINE_LENGTH      4096

#define DATA_TYPE        1  // 1: float; else: int
#define BURST_WIDTH      512
#define REAL_WIDTH       16
#define MAX_CAND         1024*16 // Maximum number of candidates to generate

#define NBOXCAR         16  // Number of boxcars to create
#define MAX_PIX         256 // Number of pixels on a side of the image
#define MAX_TIME        256 // Number of time samples to process
#define MAX_DM          1024 // Number of DMs to search

#define NREAL_PER_BURST   (BURST_WIDTH/REAL_WIDTH) // Number of PIXELS to process at a time
#define MAX_SMP_PER_IMG   (MAX_PIX*MAX_PIX) // Number of pixels in the (square) image
#define MAX_BURST_PER_IMG (MAX_SMP_PER_IMG/NREAL_PER_BURST) // Number of processing block per image

#if REAL_WIDTH == 16
#define INT_WIDTH_REAL 8
#define INT_WIDTH_ACCUM  10
//#define REAL_RNG         127
#define REAL_RNG         8
#if DATA_TYPE == 1
//typedef ap_fixed<REAL_WIDTH, INT_WIDTH_REAL,  AP_RND, AP_SAT> real_t;
//typedef ap_fixed<REAL_WIDTH, INT_WIDTH_ACCUM, AP_RND, AP_SAT> accum_t;
typedef ap_fixed<REAL_WIDTH, INT_WIDTH_REAL,  AP_RND, AP_WRAP_SM> real_t;
typedef ap_fixed<REAL_WIDTH, INT_WIDTH_ACCUM, AP_RND, AP_WRAP_SM> accum_t;
//typedef ap_fixed<REAL_WIDTH, INT_WIDTH_REAL,  AP_RND_INF, AP_WRAP_SM> real_t;
//typedef ap_fixed<REAL_WIDTH, INT_WIDTH_ACCUM, AP_RND_INF, AP_WRAP_SM> accum_t;
#else
typedef ap_int<REAL_WIDTH> real_t; // The size of this should be REAL_WIDTH
typedef ap_int<REAL_WIDTH> accum_t; // The size of this should be REAL_WIDTH
#endif
#endif

// For top-level STREAM
typedef ap_axiu<BURST_WIDTH, 0, 0, 0>  stream_burst_core;
typedef hls::stream<stream_burst_core> stream_burst;

typedef ap_uint<64> cand_krnl_t;

typedef ap_uint<BURST_WIDTH>     burst_real;
typedef hls::stream<burst_real>  fifo_burst;
typedef hls::stream<cand_krnl_t> fifo_cand;

const accum_t THRESHOLD  = accum_t(10);

typedef struct cand_t{
  accum_t  snr;
  ap_uint<16> loc_img;
  ap_uint<8>  boxcar_width;
  ap_uint<8>  time;
  ap_uint<16> dm;
}cand_t;

int boxcar(
	   const real_t *in,
           int ndm,
           int ntime,
           int nsmp_per_img,
           real_t *history_in,
           real_t *history_out,
           cand_t *out
	   );

//typedef union{
//  ap_uint<REAL_WIDTH> raw_val.range();
//  accum_t accum_val.range();
//}raw_accum;

#endif
