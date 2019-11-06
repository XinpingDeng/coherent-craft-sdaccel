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
#include <hls_stream.h>
#include <complex>
#include <ap_fixed.h>

#define NDATA_PER_SAMP 2      // We are using complex numbers
#define CORE_DATA_WIDTH 16    // We are using float complex numbers
#define BURST_DATA_WIDTH 512  // Memory width of xilinx, hard limit

#define COMPUTE_DATA_WIDTH (NDATA_PER_SAMP*CORE_DATA_WIDTH)   // complex float
#define NSAMP_PER_BURST (BURST_DATA_WIDTH/COMPUTE_DATA_WIDTH) 
#define NDATA_PER_BURST (NDATA_PER_SAMP*NSAMP_PER_BURST)

#define NANT 3
#define NCHAN 288
#define NTIME_PER_CU 10    // For each compute unit

//#define NANT 30
//#define NCHAN 288        // possible numbers: 288, 288, 672
//#define NTIME_PER_CU 256    // configurable number, depends the available memory

#define NBASELINE (NANT*(NANT-1)/2)
#define NSAMP_PER_TIME (NCHAN*NBASELINE)
#define NBURST_PER_TIME (NSAMP_PER_TIME/NSAMP_PER_BURST)
#define NBURST_PER_TIME_PER_BASELINE (NCHAN/NSAMP_PER_BURST)

//typedef float core_data_type; // The size of this should be CORE_DATA_WIDTH
typedef ap_fixed<CORE_DATA_WIDTH, CORE_DATA_WIDTH/2> core_data_type; // The size of this should be CORE_DATA_WIDTH

typedef struct burst_data_type{
  core_data_type data[NDATA_PER_BURST];
}burst_data_type; // The size of this should be BURST_DATA_WIDTH
  
typedef std::complex<core_data_type> compute_data_type; // The size of it should be COMPUTE_DATA_WIDTH

int prepare(core_data_type *in_pol1,
	    core_data_type *in_pol2,
	    core_data_type *cal_pol1,
	    core_data_type *cal_pol2,
	    core_data_type *sky,
	    core_data_type *out,
	    core_data_type *average_pol1,
	    core_data_type *average_pol2);

#define MEM_ALIGNMENT 4096
#define RAND_RANGE 4096
