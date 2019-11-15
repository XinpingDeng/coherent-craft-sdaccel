/*
******************************************************************************
** PREPARE CODE FILE
******************************************************************************
*/

#include "prepare.h"
#include "util_sdaccel.h"

int prepare(
	    core_data_type *in_pol1,
	    core_data_type *in_pol2,
	    core_data_type *cal_pol1,
	    core_data_type *cal_pol2,
	    core_data_type *sky,
	    core_data_type *out,
	    core_data_type *average_pol1,
	    core_data_type *average_pol2
	    ){
  int i;
  int j;
  int loc;
  
  compute_data_type in_pol1_tmp;
  compute_data_type in_pol2_tmp;
  compute_data_type cal_pol1_tmp;
  compute_data_type cal_pol2_tmp;
  compute_data_type sky_tmp;
  compute_data_type out_tmp;
  compute_data_type average_pol1_tmp;
  compute_data_type average_pol2_tmp;
  
  for(i = 0; i < NSAMP_PER_TIME; i++){
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
      
    for(j = 0; j < NTIME_PER_CU; j++){
      loc = j * NSAMP_PER_TIME + i;
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
