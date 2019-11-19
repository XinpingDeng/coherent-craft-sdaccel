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
#include <complex>
#include <ap_fixed.h>

#define FLOAT_DATA_TYPE     1
//#define CORE_DATA_WIDTH     32     // We use float 32-bits complex numbers
#define CORE_DATA_WIDTH     16       // We use ap_fixed 16-bits complex numbers
#define FFT_SIZE            256
#define NSAMP_PER_UV_OUT    65536    // FFT_SIZE^2
#define NSAMP_PER_UV_IN     4368

#if CORE_DATA_WIDTH == 32
#define COMPUTE_DATA_WIDTH  64     // (2*CORE_DATA_WIDTH), complex 
#define DATA_RANGE          4096
#define NSAMP_PER_BURST     8
#define NDATA_PER_BURST     16     //(2*NSAMP_PER_BURST)
#define NBURST_PER_UV_OUT   8192   // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#define NBURST_PER_UV_IN    546    // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#if FLOAT_DATA_TYPE == 1
typedef float core_data_type;
#else
typedef int core_data_type;
#endif

#elif CORE_DATA_WIDTH == 16
#define COMPUTE_DATA_WIDTH  32     // (2*CORE_DATA_WIDTH), complex 
#define DATA_RANGE          127
#define NSAMP_PER_BURST     16
#define NDATA_PER_BURST     32     //(2*NSAMP_PER_BURST)
#define NBURST_PER_UV_OUT   4096   // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#define NBURST_PER_UV_IN    273    // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#if FLOAT_DATA_TYPE == 1
#define INTEGER_WIDTH       8      // Integer width of data
typedef ap_fixed<CORE_DATA_WIDTH, INTEGER_WIDTH> core_data_type; // The size of this should be CORE_DATA_WIDTH
#else
typedef ap_int<CORE_DATA_WIDTH> core_data_type; // The size of this should be CORE_DATA_WIDTH
#endif
#endif

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef std::complex<core_data_type> compute_data_type; // The size of it should be COMPUTE_DATA_WIDTH
typedef struct burst_data_type{
  core_data_type data[NDATA_PER_BURST];
}burst_data_type; // The size of this should be 512; BURST_DATA_WIDTH

int grid(
	 core_data_type *in,
	 core_data_type *coordinate,
	 core_data_type *out,
	 int nuv_per_cu
	 );

int read_coordinate(char *fname,
		    int flen,
		    int *fdat);
