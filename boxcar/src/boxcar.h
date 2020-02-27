/*
******************************************************************************
** GRID CODE HEADER FILE
******************************************************************************
*/
#ifndef _BOXCAR_H
#define _BOXCAR_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <inttypes.h>
#include "ap_axi_sdata.h"

#define NBOXCAR          16

#define REAL_TYPE        1
//#define REAL_WIDTH      32     // We use float 32-bits real numbers
#define REAL_WIDTH       8      // We use ap_fixed 8-bits real numbers
#define BURST_WIDTH      512

#define BURST_LEN_BOXCAR 16
#define MAX_FFTSZ        256 
#define MAX_DM           1024
#define MAX_TIME         256

#define NREAL_PER_BURST   (BURST_WIDTH/REAL_WIDTH)
#define MAX_PLANE         (MAX_TIME*MAX_DM)
#define NSMP_PER_IMG      (MAX_FFTSZ*MAX_FFTSZ)
#define NBURST_PER_IMG    (NSMP_PER_IMG/NREAL_PER_BURST)
#define INT_WIDTH         (REAL_WIDTH/2)      // Integer width of data
#define MAX_BURST_HISTORY ((NBOXCAR-1)*MAX_DM*NBURST_PER_IMG)
#define MAX_BURST_HISTORY_PER_DM ((NBOXCAR-1)*NBURST_PER_IMG)

#if REAL_WIDTH == 32
#define DATA_RNG         4096
#if FLOAT == 1
typedef float real_t;
#else
typedef int real_t;
#endif

#elif REAL_WIDTH == 8
#define DATA_RNG         16
#if FLOAT == 1
typedef ap_fixed<REAL_WIDTH, INTEGER_WIDTH> real_t; // The size of this should be REAL_WIDTH
#else
typedef ap_int<REAL_WIDTH> real_t; // The size of this should be REAL_WIDTH
#endif
#endif

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef struct burst_real{
  real_t data[NREAL_PER_BURST];
}burst_real; // The size of this should be 512; BURST_REAL_WIDTH

typedef hls::stream<burst_real> fifo_burst;
typedef hls::stream<real_t>     fifo_real;
typedef ap_axiu<BURST_WIDTH, 0, 0, 0> stream_burst_core;
typedef ap_axiu<REAL_WIDTH, 0, 0, 0>  stream_real_core;
typedef hls::stream<stream_burst_core> stream_burst;
typedef hls::stream<stream_real_core>  stream_real;

int boxcar(
	   const real_t *in,
	   real_t *out1,
	   real_t *out2,
	   real_t *out3,
	   real_t *out4,
	   real_t *out5,
	   real_t *out6,
	   real_t *out7,
	   real_t *out8,
	   real_t *out9,
	   real_t *out10,
	   real_t *out11,
	   real_t *out12,
	   real_t *out13,
	   real_t *out14,
	   real_t *out15,
	   real_t *out16,
	   int ndm,
	   int ntime
	   );

real_t do_boxcar(
		 const real_t *in,
		 int dm,
		 int samp_in_img,
		 int ntime,
		 int boxcar,
		 real_t *out);

#endif
