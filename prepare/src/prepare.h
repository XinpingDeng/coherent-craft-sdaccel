/*
******************************************************************************
** PREPARE CODE HEADER FILE
******************************************************************************
*/
#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <complex>
#include <ap_fixed.h>
#include <math.h>
#include <hls_stream.h>

//#define FLOAT          1
//#define DATA_WIDTH     32     // We use float 32-bits complex numbers
#define FLOAT          1
#define DATA_WIDTH     16     // We use ap_fixed 16-bits complex numbers
//#define FLOAT          1
//#define DATA_WIDTH     8      // We use ap_fixed 8-bits complex numbers
#define MTIME_PER_CU   256
#define MCHAN          288
#define MBASELINE      435
#define MSAMP_PER_TIME (MCHAN*MBASELINE)
//#define BURST_LENGTH   54    // 288*435 = 2^5*3^3*5*29
#define BURST_LENGTH   64    // 288*435 = 2^5*3^3*5*29
#define BURST_WIDTH    512   // Memory width of xilinx, hardware limit

#define NSAMP_PER_BURST     (BURST_WIDTH/(2*DATA_WIDTH)) 
#define TILE_WIDTH          (BURST_LENGTH*NSAMP_PER_BURST)
#define INTEGER_WIDTH       (DATA_WIDTH/2)      // Integer width of data

#if DATA_WIDTH == 32
#define DATA_RANGE          4096
#if FLOAT == 1
typedef float data_t;
#else
typedef int data_t;
#endif

#elif DATA_WIDTH == 16
#define DATA_RANGE          16
#if FLOAT == 1
typedef ap_fixed<DATA_WIDTH, INTEGER_WIDTH> data_t; // The size of this should be DATA_WIDTH
#else
typedef ap_int<DATA_WIDTH> data_t; // The size of this should be DATA_WIDTH
#endif

#elif DATA_WIDTH == 8
#define DATA_RANGE          2
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

typedef std::complex<data_t> complex_t; // The size of it should be SAMP_WIDTH
typedef struct burst_t{
  data_t data[2*NSAMP_PER_BURST];
}burst_t; // The size of this should be BURST_WIDTH

typedef hls::stream<burst_t> fifo_t;

int prepare(data_t *in_pol1,
	    data_t *in_pol2,
	    data_t *cal_pol1,
	    data_t *cal_pol2,
	    data_t *sky,
	    data_t *out,
	    data_t *average_pol1,
	    data_t *average_pol2,
	    int nsamp_per_time,
	    int ntime_per_cu);
