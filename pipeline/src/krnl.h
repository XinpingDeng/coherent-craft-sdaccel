/*
******************************************************************************
** HEADER FILE FOR PIPELINE KERNELS
******************************************************************************
*/
#ifndef _KRNL_H
#define _KRNL_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ap_fixed.h>
#include <ap_int.h>
#include <assert.h>
#include <hls_stream.h>
#include "ap_axi_sdata.h"

#define DATA_TYPE          1  // 1: float; else: int
#define COORD_WIDTH        16       // Wider than the required width, but to 2^n
//#define REAL_WIDTH        32     // We use float 32-bits complex numbers
#define REAL_WIDTH         16       // We use ap_fixed 16-bits complex numbers
#define BURST_WIDTH        512
#define BURST_LEN_GRID     16
#define BURST_LEN_TRAN     16
#define BURST_LEN_PREP     16
#define BURST_LEN_WRITE    16

#define MAX_DM             1024
#define MAX_TIME           256
#define MAX_CHAN           288
#define MAX_BL             435
#define MAX_FFTSZ          256
#define MAX_SMP_PER_UV     4352

#define CMPLX_WIDTH        (2*REAL_WIDTH)
#define INT_WIDTH          (REAL_WIDTH/2)
#define NCMPLX_PER_BURST   (BURST_WIDTH/CMPLX_WIDTH)
#define NREAL_PER_BURST    (2*NCOORD_PER_BURST)
#define NCOORD_PER_BURST   NCMPLX_PER_BURST
#define TILE_WIDTH_TRAN    (BURST_LEN_TRAN*NCMPLX_PER_BURST)
#define TILE_WIDTH_PREP    (BURST_LEN_PREP*NCMPLX_PER_BURST)

#define MAX_PLANE          (MAX_DM*MAX_TIME)
#define MAX_SMP_PER_IMG    (MAX_FFTSZ*MAX_FFTSZ)    // MAX_FFTSZ^2
#define MAX_BURST_DM       (MAX_DM/NCMPLX_PER_BURST)
#define MAX_BURST_PER_IMG  (MAX_SMP_PER_IMG/NCMPLX_PER_BURST)
#define MAX_BURST_PER_UV   (MAX_SMP_PER_UV/NCMPLX_PER_BURST)

typedef ap_uint<2*REAL_WIDTH> cmplx_t; // Use for the top-level interface
typedef ap_uint<COORD_WIDTH>  coord_t; // Use inside the kernel

typedef struct burst_coord{
  coord_t data[NCOORD_PER_BURST];
}burst_coord; 

typedef struct burst_cmplx{
  cmplx_t data[NCMPLX_PER_BURST];
}burst_cmplx; // The size of this should be 512; BURST_WIDTH
typedef hls::stream<burst_cmplx> fifo_cmplx;

typedef ap_axiu<BURST_WIDTH, 0, 0, 0> stream_cmplx_core; 
typedef hls::stream<stream_cmplx_core> stream_cmplx;

#if REAL_WIDTH == 32
#define REAL_RNG           4096
#if DATA_TYPE == 1
typedef float real_t;
#else
typedef int real_t;
#endif

#elif REAL_WIDTH == 16
#define REAL_RNG          127
#if DATA_TYPE == 1
typedef ap_fixed<REAL_WIDTH, INT_WIDTH> real_t; // The size of this should be REAL_WIDTH
#else
typedef ap_int<REAL_WIDTH> real_t; // The size of this should be REAL_WIDTH
#endif
#endif

typedef struct burst_real{
  real_t data[NREAL_PER_BURST];
}burst_real; // The size of this should be 512; BURST_WIDTH
typedef hls::stream<burst_real> fifo_real;

#endif
