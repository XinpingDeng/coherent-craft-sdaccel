/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "grid.h"
#include "util_sdaccel.h"

int grid(
	 uv_data_t *in,
	 coord_t *coord,
	 uv_data_t *out,
	 int nuv_per_cu,         
         int nsamp_per_uv_in,
         int nsamp_per_uv_out
	 ){
  int i;
  int j;
  int loc_in;
  int loc_out;

  for(i = 0; i < nuv_per_cu; i++){
    for(j = 0; j < nsamp_per_uv_out; j++){
      loc_out = i*nsamp_per_uv_out + j;
      out[2*loc_out]   = 0;
      out[2*loc_out+1] = 0;
    }
    
    for(j = 0; j < nsamp_per_uv_in; j++){
      loc_out = i*nsamp_per_uv_out + coord[j];
      loc_in  = i*nsamp_per_uv_in + j;
      
      out[2*loc_out]   = in[2*loc_in]  ;
      out[2*loc_out+1] = in[2*loc_in+1];
    }    
  }
  
  return EXIT_SUCCESS;
}

int read_coord(char *fname, int flen, int *coord){
  FILE *fp = NULL;
  char line[LINE_LENGTH];
  int i;
  int coord_i;
  int coord_j;
  
  fp = fopen(fname, "r");
  if(fp == NULL){
    fprintf(stderr, "ERROR: Failed to open %s\n", fname);
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    exit(EXIT_FAILURE);
  }
  
  for(i = 0; i < flen; i++){
    fgets(line, LINE_LENGTH, fp);
    sscanf(line, "%d\t%d", &coord_i, &coord_j);
    coord[i] = coord_i*MFFT_SIZE+coord_j;
  }
  
  fclose(fp);
  return EXIT_SUCCESS;
}
