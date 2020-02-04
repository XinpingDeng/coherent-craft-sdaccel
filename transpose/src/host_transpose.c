/*
******************************************************************************
** MAIN FUNCTION
******************************************************************************
*/

#include "util_sdaccel.h"
#include "transpose.h"

int main(int argc, char* argv[]){
  // Check argument
  if (argc != 2) {
    fprintf(stderr, "ERROR: Failed to execute the program!\n");
    fprintf(stderr, "USAGE: %s xclbin\n", argv[0]);
    fprintf(stderr, "INFO: Please re-execute it with the above usage method!\n");
    return EXIT_FAILURE;
  }	

  // 4368 UV;
  // Prepare host buffers
  uint64_t ndata1;
  uint64_t ndata2;
  uint64_t ndata3;
  cl_int nsamp_per_uv_in  = 4368;
  //cl_int nsamp_per_uv_out = 3552;
  //cl_int nsamp_per_uv_out = 3328;
  //cl_int ndm_per_cu       = 1024;
  //cl_int ntime_per_cu     = 256;
  //cl_int ndm_per_cu       = 2*TILE_WIDTH;
  cl_int ndm_per_cu       = 1024;
  //cl_int nsamp_per_uv_out = 3*TILE_WIDTH;
  cl_int nsamp_per_uv_out = 3328;
  cl_int ntime_per_cu     = 256;

  if(is_hw_emulation()){
    ndm_per_cu       = 2*TILE_WIDTH;
    nsamp_per_uv_out = 2*TILE_WIDTH;
    ntime_per_cu     = 2;
  }
  if(is_sw_emulation()){
    ndm_per_cu       = 2*TILE_WIDTH;
    nsamp_per_uv_out = 2*TILE_WIDTH;
    ntime_per_cu     = 2;
  }
  ndm_per_cu = ndm_per_cu - ndm_per_cu%TILE_WIDTH;
  nsamp_per_uv_out = nsamp_per_uv_out - nsamp_per_uv_out%TILE_WIDTH;
  
  cl_int nburst_dm        = ndm_per_cu/NSAMP_PER_BURST;
  cl_int nburst_per_uv_out = nsamp_per_uv_out/NSAMP_PER_BURST;

  ndata1 = 2*nsamp_per_uv_out;
  ndata2 = 2*ntime_per_cu*ndm_per_cu*(uint64_t)nsamp_per_uv_in;
  ndata3 = 2*ntime_per_cu*ndm_per_cu*(uint64_t)nsamp_per_uv_out;
  
  uv_data_t *in = NULL;
  coord_t1 *coord = NULL;
  uv_data_t *sw_out = NULL;
  uv_data_t *hw_out = NULL;
  cl_int *coord_int = NULL;
  
  in        = (uv_data_t *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(uv_data_t));
  coord     = (coord_t1 *) aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(coord_t1));
  sw_out    = (uv_data_t *)aligned_alloc(MEM_ALIGNMENT, ndata3*sizeof(uv_data_t));
  hw_out    = (uv_data_t *)aligned_alloc(MEM_ALIGNMENT, ndata3*sizeof(uv_data_t));
  coord_int = (cl_int *)   aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(cl_int));  
  
  fprintf(stdout, "INFO: %f MB memory used on host in total\n",
	  ((ndata2 + 2*ndata3)*DATA_WIDTH + ndata1*COORD_WIDTH1)/(8*1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device in total\n",
	  ((ndata2 + ndata3)*DATA_WIDTH + ndata1*COORD_WIDTH1)/(8*1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device for raw input\n",
	  ndata2*DATA_WIDTH/(8*1024.*1024.));  
  fprintf(stdout, "INFO: %f MB memory used on device for raw output\n",
	  ndata3*DATA_WIDTH/(8*1024.*1024.));  

  FILE *fp=NULL;
  char current_dir[LINE_LENGTH];
  char fname[LINE_LENGTH];

  sprintf(current_dir, "/data/FRIGG_2/Workspace/coherent-craft-sdaccel/transpose/src");
  sprintf(fname, "%s/error.txt", current_dir);
  fp = fopen(fname, "w");

  // Prepare input
  uint64_t i;
  srand(time(NULL));
  for(i = 0; i < ndata2; i++){
    in[i] = (uv_data_t)(0.99*(rand()%DATA_RANGE));
  }
  sprintf(fname, "%s/transpose_start.txt", current_dir);
  read_coord(fname, nsamp_per_uv_out, coord_int);
  for(i = 0; i < ndata1; i++){
    coord[i] = (coord_t1)coord_int[i];
  }
  memset(sw_out, 0x00, ndata3*sizeof(uv_data_t));
  memset(hw_out, 0x00, ndata3*sizeof(uv_data_t));
  
  // Calculate on host
  cl_float cpu_elapsed_time;
  struct timespec host_start;
  struct timespec host_finish;
  clock_gettime(CLOCK_REALTIME, &host_start);
  transpose(in, coord, sw_out, nsamp_per_uv_out, ntime_per_cu, ndm_per_cu);
  fprintf(stdout, "INFO: DONE HOST EXECUTION\n");
  clock_gettime(CLOCK_REALTIME, &host_finish);
  cpu_elapsed_time = (host_finish.tv_sec - host_start.tv_sec) + (host_finish.tv_nsec - host_start.tv_nsec)/1.0E9L;

  // Get platform ID and info
  cl_int err;
  cl_uint platforms;
  cl_int get_platform_id = 0;
  cl_platform_id platform_id;
  cl_platform_id platform_ids[MAX_PALTFORMS];
  char platform_name[PARAM_VALUE_SIZE];  
  OCL_CHECK(err, err = clGetPlatformIDs(MAX_PALTFORMS, platform_ids, &platforms));
  for(i = 0; i < platforms; i++){
    OCL_CHECK(err, err = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, PARAM_VALUE_SIZE, (void *)platform_name, NULL));
    if(strcmp(platform_name, "Xilinx") == 0){
      platform_id = platform_ids[i];
      get_platform_id = 1;
      break;
    }
  }
  if(get_platform_id ==0){
    fprintf(stderr, "ERROR: Failed to get platform ID!\n");
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    return EXIT_FAILURE;
  }
  
  // Get device ID and info
  cl_uint devices;
  cl_int get_device_id = 0;
  cl_device_id device_id;
  cl_device_id device_ids[MAX_DEVICES];
  char device_name[PARAM_VALUE_SIZE];
  OCL_CHECK(err, err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR, MAX_DEVICES, device_ids, &devices));
  for(i = 0; i < devices; i++){
    OCL_CHECK(err, err = clGetDeviceInfo(device_ids[i], CL_DEVICE_NAME, PARAM_VALUE_SIZE, device_name, 0));    
    if(strstr(device_name, "u280")){
      device_id = device_ids[i];
      get_device_id = 1;
      break;
    }
  }
  if(get_device_id ==0){
    fprintf(stderr, "ERROR: Failed to get device ID!\n");
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");   
    return EXIT_FAILURE; 
  }  
  fprintf(stdout, "INFO: We will use %s!\n", device_name);

  // Create context
  cl_context context;
  OCL_CHECK(err, context = clCreateContext(0, 1, &device_id, NULL, NULL, &err));
  
  // Create command queue
  cl_command_queue queue;
  OCL_CHECK(err, queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err));
  
  // Read kernel binary into memory
  char *xclbin = argv[1];
  unsigned char *binary = NULL;
  size_t binary_size;
  fprintf(stdout, "INFO: loading xclbin %s\n", xclbin);
  binary_size = (int)load_file_to_memory(xclbin, (char **) &binary);
  if (binary_size <= 0) {
    fprintf(stderr, "ERROR: Failed to load kernel from xclbin: %s\n", xclbin);
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    return EXIT_FAILURE;
  }

  // Create program binary with kernel binary
  cl_int status;
  cl_program program;
  OCL_CHECK(err, program = clCreateProgramWithBinary(context, 1, &device_id, &binary_size, (const unsigned char **) &binary, &status, &err));
  free(binary);
  
  // Program the card with the program binary
  OCL_CHECK(err, err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL));

  // Create the kernel
  cl_kernel kernel;
  OCL_CHECK(err, kernel = clCreateKernel(program, "knl_transpose", &err));

  // Prepare device buffer
  cl_mem buffer_in;
  cl_mem buffer_coord;
  cl_mem buffer_out;
  cl_mem pt[3];

  OCL_CHECK(err, buffer_in    = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uv_data_t)*ndata2, in, &err));
  OCL_CHECK(err, buffer_coord = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(coord_t1)*ndata1, coord, &err));
  OCL_CHECK(err, buffer_out   = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uv_data_t)*ndata3, hw_out, &err));
  if (!(buffer_in&&
	buffer_coord&&
	buffer_out
	)) {
    fprintf(stderr, "ERROR: Failed to allocate device memory!\n");
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    return EXIT_FAILURE;
  }

  // Setup kernel arguments
  // To use multiple banks, this has to be before any enqueue options (e.g., clEnqueueMigrateMemObjects)
  pt[0] = buffer_in;
  pt[1] = buffer_coord;
  pt[2] = buffer_out;

  OCL_CHECK(err, err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_in));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_coord)); 
  OCL_CHECK(err, err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_out));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 3, sizeof(cl_int), &nburst_per_uv_out));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 4, sizeof(cl_int), &ntime_per_cu));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 5, sizeof(cl_int), &nburst_dm));
  
  fprintf(stdout, "INFO: DONE SETUP KERNEL\n");

  // Migrate host memory to device
  cl_int inputs = 2;
  OCL_CHECK(err, err = clEnqueueMigrateMemObjects(queue, inputs, pt, 0 ,0,NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE MEMCPY FROM HOST TO KERNEL\n");

  // Execute the kernel
  struct timespec device_start;
  struct timespec device_finish;
  cl_float kernel_elapsed_time;
  clock_gettime(CLOCK_REALTIME, &device_start);
  OCL_CHECK(err, err = clEnqueueTask(queue, kernel, 0, NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE KERNEL EXECUTION\n");
  clock_gettime(CLOCK_REALTIME, &device_finish);
  kernel_elapsed_time = (device_finish.tv_sec - device_start.tv_sec) + (device_finish.tv_nsec - device_start.tv_nsec)/1.0E9L;

  // Migrate data from device to host
  cl_int outputs = 1;
  OCL_CHECK(err, err = clEnqueueMigrateMemObjects(queue, outputs, &pt[2], CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE MEMCPY FROM KERNEL TO HOST\n");

  //// Check the result
  for(i=0;i<ndata3/2;i++){
    //if((sw_out[2*i] == hw_out[2*i])&&(sw_out[2*i+1] == hw_out[2*i+1])){
    if((sw_out[2*i] != hw_out[2*i])||(sw_out[2*i+1] != hw_out[2*i+1])){
      fprintf(fp, "ERROR: Test failed %d (%f %f) (%f %f)\n", i, sw_out[2*i].to_float(), sw_out[2*i+1].to_float(), hw_out[2*i].to_float(), hw_out[2*i+1].to_float());
    }
  }
  /*
  for(i=0;i<ndata3/2;i++){
    if((sw_out[2*i] != hw_out[2*i])||(sw_out[2*i+1] != hw_out[2*i+1])){
      fprintf(fp, "%d\n", i);
    }
  }for(i=0;i<ndata3/2;i++){
    if((sw_out[2*i] != hw_out[2*i])||(sw_out[2*i+1] != hw_out[2*i+1])){
      fprintf(fp, "ERROR ");
    }
    fprintf(fp, "%d (%f %f) (%f %f)\n", i, sw_out[2*i].to_float(), sw_out[2*i+1].to_float(), hw_out[2*i].to_float(), hw_out[2*i+1].to_float());
  }
  */
  
  fclose(fp);
  
  fprintf(stdout, "INFO: DONE RESULT CHECK\n");
  
  fprintf(stdout, "INFO: Elapsed time of CPU code is %E seconds\n", cpu_elapsed_time);
  fprintf(stdout, "INFO: Elapsed time of kernel is %E seconds\n", kernel_elapsed_time);
  
  // Cleanup
  clReleaseMemObject(buffer_in);
  clReleaseMemObject(buffer_coord);
  clReleaseMemObject(buffer_out);
  
  free(in);
  free(coord);
  free(sw_out);
  free(hw_out);
  free(coord_int);
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  fprintf(stdout, "INFO: DONE ALL\n");
  
  return EXIT_SUCCESS;
}
