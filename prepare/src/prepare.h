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

#define FULL_TEST           1
#define FLOAT_DATA_TYPE     1
//#define CORE_DATA_WIDTH     32     // We use float 32-bits complex numbers
#define CORE_DATA_WIDTH     16     // We use ap_fixed 16-bits complex numbers

#if CORE_DATA_WIDTH == 32
#define COMPUTE_DATA_WIDTH  64     // (2*CORE_DATA_WIDTH), complex 
#define NSAMP_PER_BURST     8      //(BURST_DATA_WIDTH/COMPUTE_DATA_WIDTH) 
#define NDATA_PER_BURST     16     //(2*NSAMP_PER_BURST)
#define DATA_RANGE          4096
#if FLOAT_DATA_TYPE == 1
typedef float core_data_type;
#else
typedef int core_data_type;
#endif

#elif CORE_DATA_WIDTH == 16
#define COMPUTE_DATA_WIDTH  32     // (2*CORE_DATA_WIDTH), complex 
#define NSAMP_PER_BURST     16     //(BURST_DATA_WIDTH/COMPUTE_DATA_WIDTH) 
#define NDATA_PER_BURST     32     //(2*NSAMP_PER_BURST)
#define DATA_RANGE          127
#if FLOAT_DATA_TYPE == 1
#define INTEGER_WIDTH       8      // Integer width of data
typedef ap_fixed<CORE_DATA_WIDTH, INTEGER_WIDTH> core_data_type; // The size of this should be CORE_DATA_WIDTH
#else
typedef ap_int<CORE_DATA_WIDTH> core_data_type; // The size of this should be CORE_DATA_WIDTH
#endif
#endif

#if FULL_TEST == 1
#define NCHAN               288
#define NTIME_PER_CU        256    // For each compute unit
#define NBASELINE      	    435
#else
#define NCHAN               288
#define NTIME_PER_CU        10    // For each compute unit
#define NBASELINE           15
#endif

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define BURST_DATA_WIDTH    512   // Memory width of xilinx, hardware limit
#define MAX_BURST_DATA_SIZE 32768 // Max data size in bits per transaction 4KB, hardware limit
#define MAX_BURST_LENGTH    64    // (MAX_BURST_DATA_SIZE/BURST_DATA_WIDTH)
#define MEM_ALIGNMENT       4096  // memory alignment on device

#define NSAMP_PER_TIME      	(NCHAN*NBASELINE)
#define NBURST_PER_TIME     	(NSAMP_PER_TIME/NSAMP_PER_BURST)   // It is hard to pick up remainder here
#define NTRAN_PER_TIME      	(NBURST_PER_TIME/MAX_BURST_LENGTH) // It is okay to pick up remainder here
#define NBURST_PER_TIME_REMIND  (NBURST_PER_TIME%MAX_BURST_LENGTH)

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
