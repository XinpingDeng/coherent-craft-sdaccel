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
#include <hls_stream.h>

#define BURST_LENGTH        16
#define FLOAT     1
//#define DATA_WIDTH     32     // We use float 32-bits complex numbers
#define DATA_WIDTH     16       // We use ap_fixed 16-bits complex numbers

#define BURST_WIDTH    512
#define NSAMP_PER_BURST (BURST_WIDTH/(2*DATA_WIDTH))
#define TILE_WIDTH      (BURST_LENGTH*NSAMP_PER_BURST)
#define MDM             1024
#define MBURST_DM       (MDM/NSAMP_PER_BURST)

#define MSAMP_PER_UV_IN     4368
#define MSAMP_PER_UV_OUT    3552     // 3567 round to 16, the number is not certain
#define MTIME_PER_CU        256
#define MBURST_PER_UV_OUT   (MSAMP_PER_UV_OUT/NSAMP_PER_BURST)

#define COORD_WIDTH1   16       // Wider than the required width, but to 2^n
#define COORD_WIDTH2   13       // Wide enough to cover the input index range

#define INTEGER_WIDTH       (DATA_WIDTH/2)      // Integer width of data
#if DATA_WIDTH == 32
#define DATA_RANGE          4096
#if FLOAT == 1
typedef float uv_data_t;
#else
typedef int uv_data_t;
#endif

#elif DATA_WIDTH == 16
#define DATA_RANGE          127
#if FLOAT == 1
typedef ap_fixed<DATA_WIDTH, INTEGER_WIDTH> uv_data_t; // The size of this should be DATA_WIDTH
#else
typedef ap_int<DATA_WIDTH> uv_data_t; // The size of this should be DATA_WIDTH
#endif

#elif DATA_WIDTH == 8
#define DATA_RANGE          64
#if FLOAT == 1
typedef ap_fixed<DATA_WIDTH, INTEGER_WIDTH> uv_data_t; // The size of this should be DATA_WIDTH
#else
typedef ap_int<DATA_WIDTH> uv_data_t; // The size of this should be DATA_WIDTH
#endif
#endif

typedef ap_uint<2*DATA_WIDTH> uv_t;
typedef ap_uint<COORD_WIDTH1> coord_t1; // Use for the top-level interface
typedef ap_uint<COORD_WIDTH2> coord_t2; // Use inside the kernel for the index

typedef struct burst_coord{
  coord_t1 data[NSAMP_PER_BURST];
}burst_coord; 

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef struct burst_uv{
  uv_t data[NSAMP_PER_BURST];
}burst_uv; // The size of this should be 512; BURST_DATA_WIDTH

typedef hls::stream<burst_uv> fifo_uv;

int transpose(
              uv_data_t *in,
              uv_data_t *out,
              int nsamp_per_uv_out,
              int ntime_per_cu,
              int ndm_per_cu);
