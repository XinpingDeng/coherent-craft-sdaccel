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
#include <assert.h>

#define FLOAT_DATA_TYPE     1
//#define CORE_DATA_WIDTH     32     // We use float 32-bits complex numbers
#define CORE_DATA_WIDTH     16       // We use ap_fixed 16-bits complex numbers
#define FFT_SIZE            256
#define NSAMP_PER_UV_OUT    65536    // FFT_SIZE^2
#define NSAMP_PER_UV_IN     4368
#define NDATA_PER_UV_IN     8736
#define COORD_DATA_WIDTH1   16       // Wider than the required width, but to 2^n
#define COORD_DATA_WIDTH2   13       // Wide enough to cover the input index range
#define COORD_DATA_WIDTH3   3        // Wide enough to cover the counter, assume that maximum is 8
#define NBURST_BUFFER       3        // Assume that maximum in one output burst is 2 NDATA_PER_BURST
#define NSAMP_PER_CELL      8        // Assume that the maximum is 8 samples go to one single output cell
#if CORE_DATA_WIDTH == 32
#define COMPUTE_DATA_WIDTH  64     // (2*CORE_DATA_WIDTH), complex 
#define DATA_RANGE          4096
#define NSAMP_PER_BURST     8
#define NDATA_PER_BURST     16     //(2*NSAMP_PER_BURST)
#define NBURST_PER_UV_OUT   8192   // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#define NBURST_PER_UV_IN    546    // NSAMP_PER_UV_OUT/NSAMP_PER_BURST
#if FLOAT_DATA_TYPE == 1
typedef float uv_t;
#else
typedef int uv_t;
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
typedef ap_fixed<CORE_DATA_WIDTH, INTEGER_WIDTH> uv_t; // The size of this should be CORE_DATA_WIDTH
#else
typedef ap_int<CORE_DATA_WIDTH> uv_t; // The size of this should be CORE_DATA_WIDTH
#endif
#endif

typedef ap_uint<COORD_DATA_WIDTH1> coord_t1; // Use for the top-level interface
typedef ap_uint<COORD_DATA_WIDTH2> coord_t2; // Use inside the kernel for the index start
typedef ap_uint<COORD_DATA_WIDTH3> coord_t3; // Use inside the kernel for the counter

typedef struct burst_coord{
  coord_t1 data[NDATA_PER_BURST];
}burst_coord; 

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef struct burst_uv{
  uv_t data[NDATA_PER_BURST];
}burst_uv; // The size of this should be 512; BURST_DATA_WIDTH

int grid(
	 uv_t *in,
	 coord_t1 *coord,
	 uv_t *out,
	 int nuv_per_cu
	 );

int read_coord(
	       char *fname,
	       int flen,
	       int *fdat);
