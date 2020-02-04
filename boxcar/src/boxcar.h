/*
******************************************************************************
** GRID CODE HEADER FILE
******************************************************************************
*/
#pragma once

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

#define NBOXCAR             16
#define FLOAT     1
//#define DATA_WIDTH     32     // We use float 32-bits real numbers
#define DATA_WIDTH     8       // We use ap_fixed 8-bits real numbers
#define NSAMP_PER_IMG       65536    // 256^2
#define BURST_LENGTH    64
#define BURST_WIDTH     512
#define NSAMP_PER_BURST (BURST_WIDTH/DATA_WIDTH)
#define NBURST_PER_IMG  (NSAMP_PER_IMG/NSAMP_PER_BURST)

#define INTEGER_WIDTH       (DATA_WIDTH/2)      // Integer width of data

#if DATA_WIDTH == 32
#define DATA_RANGE          4096
#if FLOAT == 1
typedef float data_t;
#else
typedef int data_t;
#endif

#elif DATA_WIDTH == 8
#define DATA_RANGE          10     // Careful with the range *****
#if FLOAT == 1
typedef ap_fixed<DATA_WIDTH, INTEGER_WIDTH> data_t; // The size of this should be DATA_WIDTH
#else
typedef ap_int<DATA_WIDTH> data_t; // The size of this should be DATA_WIDTH
#endif
#endif

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef struct burst_t{
  data_t data[NSAMP_PER_BURST];
}burst_t; // The size of this should be 512; BURST_DATA_WIDTH

typedef hls::stream<burst_t> fifo_burst_t;
typedef hls::stream<data_t> fifo_data_t;

int boxcar(
	   const data_t *in,
	   data_t *out1,
	   data_t *out2,
	   data_t *out3,
	   data_t *out4,
	   data_t *out5,
	   data_t *out6,
	   data_t *out7,
	   data_t *out8,
	   data_t *out9,
	   data_t *out10,
	   data_t *out11,
	   data_t *out12,
	   data_t *out13,
	   data_t *out14,
	   data_t *out15,
	   data_t *out16,
	   int ndm,
	   int ntime
	   );

data_t do_boxcar(
		 const data_t *in,
		 int dm,
		 int samp_in_img,
		 int ntime,
		 int boxcar,
		 data_t *out);
