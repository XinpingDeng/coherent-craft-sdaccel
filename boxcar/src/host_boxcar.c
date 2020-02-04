/*
******************************************************************************
** MAIN FUNCTION
******************************************************************************
*/

#include "util_sdaccel.h"
#include "boxcar.h"

int main(int argc, char* argv[]){
  // Check argument
  if (argc != 2) {
    fprintf(stderr, "ERROR: Failed to execute the program!\n");
    fprintf(stderr, "USAGE: %s xclbin\n", argv[0]);
    fprintf(stderr, "INFO: Please re-execute it with the above usage method!\n");
    return EXIT_FAILURE;
  }	

  // Prepare host buffers
  int i;
  uint64_t j;
  uint64_t ndata0;
  uint64_t ndata[NBOXCAR];
  cl_int ndm   = 16;
  cl_int ntime = 64;
  if(is_hw_emulation()){
    ndm   = 2;
    ntime = 32;
  }
  if(is_sw_emulation()){
    ndm   = 2;
    ntime = 32;
  }
  ndata0 = ndm*(uint64_t)NSAMP_PER_IMG*ntime;
  
  data_t *in      = NULL;
  data_t **sw_out = NULL;
  data_t **hw_out = NULL;  
  in = (data_t *)aligned_alloc(MEM_ALIGNMENT, ndata0*sizeof(data_t));
  sw_out = (data_t **)calloc(NBOXCAR, sizeof(data_t*));
  hw_out = (data_t **)calloc(NBOXCAR, sizeof(data_t*));
  for(i = 0; i < NBOXCAR; i++){
    ndata[i] = ndm*(uint64_t)NSAMP_PER_IMG*(ntime-i);
    sw_out[i] = (data_t *)aligned_alloc(MEM_ALIGNMENT, ndata0*sizeof(data_t));
    hw_out[i] = (data_t *)aligned_alloc(MEM_ALIGNMENT, ndata0*sizeof(data_t));
    memset(sw_out[i], 0x00, ndata0*sizeof(data_t));
    memset(hw_out[i], 0x00, ndata0*sizeof(data_t));
  }    
  fprintf(stdout, "INFO: %f MB memory used on host in total\n",
	  (2*NBOXCAR+1)*ndata0*DATA_WIDTH/(8*1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device in total\n",
	  (NBOXCAR+1)*ndata0*DATA_WIDTH/(8*1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device for raw input\n",
	  ndata0*DATA_WIDTH/(8*1024.*1024.));  
  fprintf(stdout, "INFO: %f MB memory used on device for raw output\n",
	  ndata0*NBOXCAR*DATA_WIDTH/(8*1024.*1024.));  
  
  // Prepare input
  srand(time(NULL));
  for(j = 0; j < ndata0; j++){
    in[j] = (data_t)(0.99*(rand()%DATA_RANGE));
  }
  
  // Calculate on host
  cl_float cpu_elapsed_time;
  struct timespec host_start;
  struct timespec host_finish;
  clock_gettime(CLOCK_REALTIME, &host_start);
  boxcar(in, sw_out[0], sw_out[1], sw_out[2], sw_out[3],
	 sw_out[4], sw_out[5], sw_out[6], sw_out[7],
	 sw_out[8], sw_out[9], sw_out[10], sw_out[11],
	 sw_out[12], sw_out[13], sw_out[14], sw_out[15],
	 ndm, ntime);
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
  OCL_CHECK(err, kernel = clCreateKernel(program, "knl_boxcar", &err));

  // Prepare device buffer
  cl_mem buffer_in;
  cl_mem buffer_out[NBOXCAR];
  cl_mem pt[NBOXCAR+1];
  status = 1;
  OCL_CHECK(err, buffer_in   = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR, sizeof(data_t)*ndata0, in, &err));
  status = status && buffer_in;
  for(i = 0; i < NBOXCAR; i++){
    OCL_CHECK(err, buffer_out[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(data_t)*ndata0, hw_out[i], &err));
    status = status && hw_out[i];
  }
  if (!status) {
    fprintf(stderr, "ERROR: Failed to allocate device memory!\n");
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    return EXIT_FAILURE;
  }
  
  // Setup kernel arguments
  // To use multiple banks, this has to be before any enqueue options (e.g., clEnqueueMigrateMemObjects)
  pt[0] = buffer_in;
  for(i = 0; i < NBOXCAR; i++){
    pt[i+1] = buffer_out[i];
  }
  OCL_CHECK(err, err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_in));
  for(i = 0; i < NBOXCAR; i++){
    OCL_CHECK(err, err = clSetKernelArg(kernel, i+1, sizeof(cl_mem), &buffer_out[i]));
  }
  OCL_CHECK(err, err = clSetKernelArg(kernel, NBOXCAR+1, sizeof(cl_int), &ndm));
  OCL_CHECK(err, err = clSetKernelArg(kernel, NBOXCAR+2, sizeof(cl_int), &ntime));
  fprintf(stdout, "INFO: DONE SETUP KERNEL\n");

  // Migrate host memory to device
  cl_int inputs = 1;
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
  cl_int outputs = NBOXCAR;
  OCL_CHECK(err, err = clEnqueueMigrateMemObjects(queue, outputs, &pt[1], CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE MEMCPY FROM KERNEL TO HOST\n");

  // Save result to files for further check
  FILE *fp=NULL;
  char fname[LINE_LENGTH];
  for(i = 0; i < NBOXCAR; i++){
    sprintf(fname, "/data/FRIGG_2/Workspace/coherent-craft-sdaccel/boxcar/src/out%d.txt", i);
    fp = fopen(fname, "w");
    for(j = 0; j < ndata0; j++) {
      fprintf(fp, "%f\n", sw_out[i][j].to_float());
    }
    fclose(fp);
  }
	
  // Check the result
  uint64_t counter;
  for(i = 0; i < NBOXCAR; i++){
    counter = 0;
    for(j = 0; j<ndata[i]; j++){
      if(sw_out[i][j]!=0){
	counter++;
      }    
      if(sw_out[i][j] != hw_out[i][j]){
	fprintf(stderr, "ERROR: Test failed %"PRIu64" (%f %f)\n", i, sw_out[i][j].to_float(), hw_out[i][j].to_float());
	break;
      }
    }
    fprintf(stdout, "INFO: %"PRIu64" none zero boxcar%d out of %"PRIu64", which is %.0f\%\n", counter, i+1, ndata[i], 100.0*(float)counter/ndata[i]);
  }  
  fprintf(stdout, "INFO: DONE RESULT CHECK\n");
  fprintf(stdout, "INFO: Elapsed time of CPU code is %E seconds\n", cpu_elapsed_time);
  fprintf(stdout, "INFO: Elapsed time of kernel is %E seconds\n", kernel_elapsed_time);
  
  // Cleanup
  clReleaseMemObject(buffer_in);
  for(i = 0; i < NBOXCAR; i++){
    clReleaseMemObject(buffer_out[i]);
  }
  
  free(in);
  for(i = 0; i < NBOXCAR; i++){
    free(sw_out[i]);
    free(hw_out[i]);
  }
  
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  fprintf(stdout, "INFO: DONE ALL\n");
  
  return EXIT_SUCCESS;
}
