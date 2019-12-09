/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "boxcar.h"
#include "util_sdaccel.h"

int boxcar(
	   const core_t *in,
	   core_t *out1,
	   core_t *out2,
	   core_t *out3,
	   core_t *out4,
	   core_t *out5,
	   core_t *out6,
	   core_t *out7,
	   core_t *out8,
	   core_t *out9,
	   core_t *out10,
	   core_t *out11,
	   core_t *out12,
	   core_t *out13,
	   core_t *out14,
	   core_t *out15,
	   core_t *out16,
	   int ndm,
	   int ntime
	   ){
  int i;
  int j;
  int m;
  long loc_in;
  long loc_out;
  
  for(i = 0; i < ndm; i++){
    for(j = 0; j < NSAMP_PER_IMG; j++){
      // boxcar1
      for(m = 0; m < ntime; m++){
	loc_in  = i*ntime*NSAMP_PER_IMG + m*NSAMP_PER_IMG + j;
	loc_out = loc_in;	
	out1[loc_out] = in[loc_in];
      }
      
      // Boxcar2
      do_boxcar(in, i, j, ntime, 2, out2);
      
      // Boxcar3
      do_boxcar(in, i, j, ntime, 3, out3);
      
      // Boxcar4
      do_boxcar(in, i, j, ntime, 4, out4);
      
      // Boxcar5
      do_boxcar(in, i, j, ntime, 5, out5);
      
      // Boxcar6
      do_boxcar(in, i, j, ntime, 6, out6);
      
      // Boxcar7
      do_boxcar(in, i, j, ntime, 7, out7);
      
      // Boxcar8
      do_boxcar(in, i, j, ntime, 8, out8);
      
      // Boxcar9
      do_boxcar(in, i, j, ntime, 9, out9);
      
      // Boxcar10
      do_boxcar(in, i, j, ntime, 10, out10);
      
      // Boxcar11
      do_boxcar(in, i, j, ntime, 11, out11);
      
      // Boxcar12
      do_boxcar(in, i, j, ntime, 12, out12);
      
      // Boxcar13
      do_boxcar(in, i, j, ntime, 13, out13);
      
      // Boxcar14
      do_boxcar(in, i, j, ntime, 14, out14);
      
      // Boxcar15
      do_boxcar(in, i, j, ntime, 15, out15);
      
      // Boxcar16
      do_boxcar(in, i, j, ntime, 16, out16);
    }
  }
  return EXIT_SUCCESS;
}


core_t do_boxcar(
		 const core_t *in,
		 int dm,
		 int samp_in_img,
		 int ntime,
		 int boxcar,
		 core_t *out){
  core_t result;
  core_t previous;
  int i;
  long loc_out;
  long loc_in;
  
  result = 0;
  loc_in = dm*ntime*NSAMP_PER_IMG + samp_in_img;
  previous = in[loc_in];
  for(i = 0; i < boxcar; i++){
    loc_in = dm*ntime*NSAMP_PER_IMG + i*NSAMP_PER_IMG + samp_in_img;
    result += in[loc_in];	
  }
  loc_out      = dm*(ntime+1-boxcar)*NSAMP_PER_IMG + samp_in_img;
  out[loc_out] = result;      
  for(i = boxcar; i < ntime; i++){
    loc_in   = dm*ntime*NSAMP_PER_IMG + i*NSAMP_PER_IMG + samp_in_img;
    result   += in[loc_in];
    result   -= previous;
    
    loc_out      = dm*(ntime+1-boxcar)*NSAMP_PER_IMG + (i+1-boxcar)*NSAMP_PER_IMG + samp_in_img;
    out[loc_out] = result;
    
    loc_in   = dm*ntime*NSAMP_PER_IMG + (i+1-boxcar)*NSAMP_PER_IMG + samp_in_img;
    previous = in[loc_in];
  }
  
  return EXIT_SUCCESS;
}
