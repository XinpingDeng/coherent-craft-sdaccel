/*
******************************************************************************
** HEADER FILE FOR PREPARE
******************************************************************************
*/

#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <complex>

#define NDATA_PER_POL 2
#define BASE_DATA_WIDTH 32
#define PORT_DATA_WIDTH 512 // Assume that single port data width for single pol
#define POL_DATA_WIDTH (NDATA_PER_POL*BASE_DATA_WIDTH)

#define NSAMP_PER_BURST (PORT_DATA_WIDTH/POL_DATA_WIDTH)
#define NDATA_PER_BURST_1POL (NDATA_PER_POL*NSAMP_PER_BURST)
#define NDATA_PER_BURST_2POL (2*NDATA_PER_BURST_1POL)

//#define NANT 4
//#define NCHAN 256
//#define NTIME_PER_CU 2    // configurable number, depends the available memory

#define NANT 30
#define NCHAN 288        // possible numbers: 288, 288, 672
#define NTIME_PER_CU 64    // configurable number, depends the available memory

#define NBASELINE (NANT*(NANT-1)/2)
#define NSAMP_PER_TIME (NCHAN*NBASELINE)
#define NBURST_PER_TIME (NSAMP_PER_TIME/NSAMP_PER_BURST)
  
typedef float base_data_type;
typedef std::complex<base_data_type> complex_data_type;

#define MEM_ALIGNMENT (sizeof(complex_data_type))//4096
#define RAND_RANGE 4096

typedef struct burst_data_1pol_type {
  base_data_type data[NDATA_PER_BURST_1POL];
} burst_data_1pol_type;

typedef struct burst_data_2pol_type {
  base_data_type data[NDATA_PER_BURST_2POL];
} burst_data_2pol_type;

int prepare(
	    complex_data_type *in,
	    complex_data_type *cal,
	    complex_data_type *sky,
	    complex_data_type *out,
	    complex_data_type *average);
