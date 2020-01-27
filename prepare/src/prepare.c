/*
******************************************************************************
** PREPARE CODE FILE
******************************************************************************
*/

#include "prepare.h"
#include "util_sdaccel.h"

int prepare(
	    data_t *in_pol1,
	    data_t *in_pol2,
	    data_t *cal_pol1,
	    data_t *cal_pol2,
	    data_t *sky,
	    data_t *out,
	    data_t *average_pol1,
	    data_t *average_pol2,
	    int nsamp_per_time,
	    int ntime_per_cu
	    ){
  int i;
  int j;
  int loc;
  
  complex_t in_pol1_tmp;
  complex_t in_pol2_tmp;
  complex_t cal_pol1_tmp;
  complex_t cal_pol2_tmp;
  complex_t sky_tmp;
  complex_t out_tmp;
  complex_t average_pol1_tmp;
  complex_t average_pol2_tmp;
  
  for(i = 0; i < nsamp_per_time; i++){
    sky_tmp.real(sky[2*i]);
    sky_tmp.imag(sky[2*i+1]);

    cal_pol1_tmp.real(cal_pol1[2*i]);
    cal_pol1_tmp.imag(cal_pol1[2*i+1]);
    cal_pol2_tmp.real(cal_pol2[2*i]);
    cal_pol2_tmp.imag(cal_pol2[2*i+1]);

    average_pol1_tmp.real(0);
    average_pol1_tmp.imag(0);
    average_pol2_tmp.real(0);
    average_pol2_tmp.imag(0);
      
    for(j = 0; j < ntime_per_cu; j++){
      loc = j * nsamp_per_time + i;
      in_pol1_tmp.real(in_pol1[2*loc]);
      in_pol1_tmp.imag(in_pol1[2*loc+1]);
	       
      in_pol2_tmp.real(in_pol2[2*loc]);
      in_pol2_tmp.imag(in_pol2[2*loc+1]);

      average_pol1_tmp += in_pol1_tmp;
      average_pol2_tmp += in_pol2_tmp;
	
      out_tmp = in_pol1_tmp * cal_pol1_tmp +
	in_pol2_tmp * cal_pol2_tmp -
	sky_tmp;
	
      out[2*loc]   = out_tmp.real();
      out[2*loc+1] = out_tmp.imag();
    }
      
    average_pol1[2*i]   = average_pol1_tmp.real();
    average_pol1[2*i+1] = average_pol1_tmp.imag();
    average_pol2[2*i]   = average_pol2_tmp.real();
    average_pol2[2*i+1] = average_pol2_tmp.imag();
  }
  
  return EXIT_SUCCESS;
}
