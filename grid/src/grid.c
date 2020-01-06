/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "grid.h"
#include "util_sdaccel.h"

int grid(
	 uv_t *in,
	 coord_t1 *coord,
	 uv_t *out,
	 int nuv_per_cu
	 ){
  int i;
  int j;
  int k;
  int loc_in;
  int loc_out;
  int start;
  int count;
  
  for(i = 0; i < nuv_per_cu; i++){
    for(j = 0; j < NSAMP_PER_UV_OUT; j++){
      loc_out = i*NSAMP_PER_UV_OUT + j;
      out[2*loc_out]   = 0;
      out[2*loc_out+1] = 0;
      count = coord[2*j+1];
      if(count!=0){
        start = coord[2*j];
        for(k = 0; k < count; k++){
          loc_in  = i*NSAMP_PER_UV_IN + start + k;
          out[2*loc_out]   += in[2*loc_in];
          out[2*loc_out+1] += in[2*loc_in+1];
        }
      }
    }
  }
  
  return EXIT_SUCCESS;
}

int read_coord(char *fname, int flen, int *fdat){
  FILE *fp = NULL;
  char line[LINE_LENGTH];
  fp = fopen(fname, "r");
  if(fp == NULL){
    fprintf(stderr, "ERROR: Failed to open %s\n", fname);
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    exit(EXIT_FAILURE);
  }
  for(int i = 0; i < flen; i++){
    fgets(line, LINE_LENGTH, fp);
    sscanf(line, "%d\t%d", &fdat[2*i], &fdat[2*i+1]);
  }
  
  fclose(fp);
  return EXIT_SUCCESS;
}
