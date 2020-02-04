/*
******************************************************************************
** GRID CODE FILE
******************************************************************************
*/

#include "transpose.h"
#include "util_sdaccel.h"

int transpose(
              uv_data_t *in,
              coord_t1 *coord,
              uv_data_t *out,
              int nsamp_per_uv_out,
              int ntime_per_cu,
              int ndm_per_cu){
  int i;
  int j;
  int k;
  int loc_in;
  int loc_out;
  
  for(i = 0; i < ntime_per_cu; i++){
    for(j = 0; j < nsamp_per_uv_out; j++){
      for(k = 0; k < ndm_per_cu; k++){
        loc_in  = coord[j]*ntime_per_cu*ndm_per_cu + i * ndm_per_cu + k;
        loc_out = k*nsamp_per_uv_out*ntime_per_cu + i * nsamp_per_uv_out + j;
        
        out[2*loc_out]   = in[2*loc_in];
        out[2*loc_out+1] = in[2*loc_in+1];
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
    sscanf(line, "%d", &fdat[i]);
  }
  
  fclose(fp);
  return EXIT_SUCCESS;
}
