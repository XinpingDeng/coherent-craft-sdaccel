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

#define FLOAT_DATA_TYPE     1
//#define CORE_DATA_WIDTH     32     // We use float 32-bits real numbers
#define CORE_DATA_WIDTH     8       // We use ap_fixed 8-bits real numbers
#define NSAMP_PER_IMG        65536    // 256^2

#if CORE_DATA_WIDTH == 32
#define COMPUTE_DATA_WIDTH  64     // (2*CORE_DATA_WIDTH), complex 
#define DATA_RANGE          4096
#define NSAMP_PER_BURST     16
#define NBURST_PER_IMG      8192   // NSAMP_PER_IMG_OUT/NSAMP_PER_BURST
#if FLOAT_DATA_TYPE == 1
typedef float img_t;
#else
typedef int img_t;
#endif

#elif CORE_DATA_WIDTH == 8
#define COMPUTE_DATA_WIDTH  16     // (2*CORE_DATA_WIDTH), complex 
#define DATA_RANGE          63
#define NSAMP_PER_BURST     64
#define NBURST_PER_IMG      1024   // NSAMP_PER_IMG_OUT/NSAMP_PER_BURST
#if FLOAT_DATA_TYPE == 1
#define INTEGER_WIDTH       6      // Integer width of data
typedef ap_fixed<CORE_DATA_WIDTH, INTEGER_WIDTH> img_t; // The size of this should be CORE_DATA_WIDTH
#else
typedef ap_int<CORE_DATA_WIDTH> img_t; // The size of this should be CORE_DATA_WIDTH
#endif
#endif

#define MAX_PALTFORMS       16
#define MAX_DEVICES         16
#define PARAM_VALUE_SIZE    1024
#define MEM_ALIGNMENT       4096  // memory alignment on device
#define LINE_LENGTH         4096

typedef struct burst_img{
  img_t data[NSAMP_PER_BURST];
}burst_img; // The size of this should be 512; BURST_DATA_WIDTH

typedef hls::stream<burst_img> stream_img;
