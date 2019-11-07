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

#define NDATA_PER_SAMP      	2      // We are using complex numbers
#define CORE_DATA_WIDTH     	16    // We are using float complex numbers
#define COMPUTE_DATA_WIDTH  	(NDATA_PER_SAMP*CORE_DATA_WIDTH)   // complex float
			    	
#define BURST_DATA_WIDTH    	512  // Memory width of xilinx, hard limit
#define MAX_BURST_DATA_SIZE 	32768 // Max data size in bits per transaction 4KB
#define MAX_BURST_LENGTH    	(MAX_BURST_DATA_SIZE/BURST_DATA_WIDTH)
			    	
#define NSAMP_PER_BURST     	(BURST_DATA_WIDTH/COMPUTE_DATA_WIDTH) 
#define NDATA_PER_BURST     	(NDATA_PER_SAMP*NSAMP_PER_BURST)
			    	
#define NCHAN             	  288
#define NTIME_PER_CU      	  256    // For each compute unit
#define NBASELINE         	  435
			    	
//#define NCHAN               	288
//#define NTIME_PER_CU        	10    // For each compute unit
//#define NBASELINE           	15
			    	
#define NSAMP_PER_TIME      	(NCHAN*NBASELINE)
#define NBURST_PER_TIME     	(NSAMP_PER_TIME/NSAMP_PER_BURST)   // It is hard to pick up remainder here
#define NTRAN_PER_TIME      	(NBURST_PER_TIME/MAX_BURST_LENGTH) // It is okay to pick up remainder here
#define NBURST_PER_TIME_REMIND  (NBURST_PER_TIME%MAX_BURST_LENGTH)

typedef ap_fixed<CORE_DATA_WIDTH, CORE_DATA_WIDTH/2> core_data_type; // The size of this should be CORE_DATA_WIDTH
typedef std::complex<core_data_type> compute_data_type; // The size of it should be COMPUTE_DATA_WIDTH

typedef struct burst_data_type{
  core_data_type data[NDATA_PER_BURST];
}burst_data_type; // The size of this should be BURST_DATA_WIDTH

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
