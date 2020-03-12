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
  cl_int npix  = 256;
  cl_int ndm   = 16;
  cl_int ntime = 64;  
  if(is_hw_emulation()){
    ndm   = 2;
    ntime = 2;
    npix  = 8;
  }
  if(is_sw_emulation()){
    ndm   = 2;
    ntime = 2;
    npix  = 8;
  }
  cl_int nsmp_per_img    = npix*npix;
  cl_int nburst_per_img  = nsmp_per_img/NREAL_PER_BURST;
  uint64_t ndata_in      = ndm*ntime*nsmp_per_img;
  uint64_t ndata_history = ndm*nsmp_per_img*(NBOXCAR-1);
  
  real_t *in             = NULL;
  real_t *history_in     = NULL;
  real_t *sw_history_out = NULL;
  real_t *hw_history_out = NULL;
  cand_t *hw_cand        = NULL;  
  cand_t *sw_cand        = NULL;
  
  in             = (real_t *)aligned_alloc(MEM_ALIGNMENT, ndata_in*sizeof(real_t));
  history_in     = (real_t *)aligned_alloc(MEM_ALIGNMENT, ndata_history*sizeof(real_t));
  sw_history_out = (real_t *)aligned_alloc(MEM_ALIGNMENT, ndata_history*sizeof(real_t));
  hw_history_out = (real_t *)aligned_alloc(MEM_ALIGNMENT, ndata_history*sizeof(real_t));
  sw_cand        = (cand_t *)aligned_alloc(MEM_ALIGNMENT, MAX_CAND*sizeof(cand_t));
  hw_cand        = (cand_t *)aligned_alloc(MEM_ALIGNMENT, MAX_CAND*sizeof(cand_t));
  
  fprintf(stdout, "INFO: %f MB memory used on host in total\n",
	  ((ndata_in + 3*ndata_history)*sizeof(real_t)
           + 2*MAX_CAND*sizeof(cand_t))
          /(1024.*1024.)
          );
  fprintf(stdout, "INFO: %f MB memory used on device in total\n",
	  ((ndata_in + 2*ndata_history)*sizeof(real_t)
           + MAX_CAND*sizeof(cand_t))
          /(1024.*1024.)
          );
  fprintf(stdout, "INFO: %f MB memory used on device for raw input\n",
	  ((ndata_in + ndata_history)*sizeof(real_t))
          /(1024.*1024.)
          );
  fprintf(stdout, "INFO: %f MB memory used on device for raw output\n",
	  (ndata_history*sizeof(real_t)
           + MAX_CAND*sizeof(cand_t))
          /(1024.*1024.)
          );
  
  // Prepare input
  int i;
  srand(time(NULL));
  for(i = 0; i < ndata_in; i++){
    in[i] = (real_t)(0.99*(rand()%REAL_RNG));
    fprintf(stdout, "INPUT from host %f\n", in[i].to_float());
  }
  for(i = 0; i < ndata_history; i++){
    history_in[i] = (real_t)(0.99*(rand()%REAL_RNG));
  }
  
  // Calculate on host
  cl_float cpu_elapsed_time;
  struct timespec host_start;
  struct timespec host_finish;
  clock_gettime(CLOCK_REALTIME, &host_start);
  boxcar(in, 
	 ndm,
         ntime,
         nsmp_per_img,
         history_in,
         sw_history_out,
         sw_cand);
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
  cl_kernel kernel_boxcar;
  cl_kernel kernel_stream;
  OCL_CHECK(err, kernel_boxcar = clCreateKernel(program, "krnl_boxcar", &err));
  OCL_CHECK(err, kernel_stream = clCreateKernel(program, "krnl_stream", &err));

  // Prepare device buffer
  cl_mem buffer_in;
  cl_mem buffer_history_in;
  cl_mem buffer_history_out;
  cl_mem buffer_cand;
  cl_mem pt[4];
  status = 1;
  OCL_CHECK(err, buffer_in          = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR, sizeof(real_t)*ndata_in, in, &err));
  OCL_CHECK(err, buffer_history_in  = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR, sizeof(real_t)*ndata_history, history_in, &err));
  OCL_CHECK(err, buffer_history_out = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR, sizeof(real_t)*ndata_history, hw_history_out, &err));
  OCL_CHECK(err, buffer_cand        = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR, sizeof(cand_t)*MAX_CAND, hw_cand, &err));
  status = status && buffer_in && buffer_history_in && buffer_history_out && buffer_cand;
  if (!status) {
    fprintf(stderr, "ERROR: Failed to allocate device memory!\n");
    fprintf(stderr, "ERROR: Please look into the file \"%s\" above line [%d]!\n", __FILE__, __LINE__);
    fprintf(stderr, "ERROR: Test failed ...!\n");
    return EXIT_FAILURE;
  }
  
  // Setup kernel arguments
  // To use multiple banks, this has to be before any enqueue options (e.g., clEnqueueMigrateMemObjects)
  pt[0] = buffer_in;
  pt[1] = buffer_history_in;
  pt[2] = buffer_history_out;
  pt[3] = buffer_cand;
  
  OCL_CHECK(err, err = clSetKernelArg(kernel_stream, 0, sizeof(cl_mem), &buffer_in));
  OCL_CHECK(err, err = clSetKernelArg(kernel_stream, 1, sizeof(cl_int), &ndm));
  OCL_CHECK(err, err = clSetKernelArg(kernel_stream, 2, sizeof(cl_int), &ntime));
  OCL_CHECK(err, err = clSetKernelArg(kernel_stream, 3, sizeof(cl_int), &nburst_per_img));
  
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 1, sizeof(cl_int), &ndm));
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 2, sizeof(cl_int), &ntime));
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 3, sizeof(cl_int), &nburst_per_img));
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 4, sizeof(cl_mem), &buffer_history_in));
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 5, sizeof(cl_mem), &buffer_history_out));
  OCL_CHECK(err, err = clSetKernelArg(kernel_boxcar, 6, sizeof(cl_mem), &buffer_cand));
  
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
  OCL_CHECK(err, err = clEnqueueTask(queue, kernel_stream, 0, NULL, NULL));
  OCL_CHECK(err, err = clEnqueueTask(queue, kernel_boxcar, 0, NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE KERNEL EXECUTION\n");
  clock_gettime(CLOCK_REALTIME, &device_finish);
  kernel_elapsed_time = (device_finish.tv_sec - device_start.tv_sec) + (device_finish.tv_nsec - device_start.tv_nsec)/1.0E9L;

  // Migrate data from device to host
  cl_int outputs = 2;
  OCL_CHECK(err, err = clEnqueueMigrateMemObjects(queue, outputs, &pt[2], CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL, NULL));
  OCL_CHECK(err, err = clFinish(queue));
  fprintf(stdout, "INFO: DONE MEMCPY FROM KERNEL TO HOST\n");

  // Save result to files for further check
  FILE *fp=NULL;
  
  fp = fopen("/data/FRIGG_2/Workspace/coherent-craft-sdaccel/boxcar/src/out_host.txt", "w");
  for(i = 0; i < MAX_CAND; i++) {
    if(sw_cand[i].snr.to_float()!=0){
      fprintf(fp, "%f\t%d\t%d\t%d\t%d\n", sw_cand[i].snr.to_float(), (int)sw_cand[i].loc_img, (int)sw_cand[i].boxcar_width, (int)sw_cand[i].time, (int)sw_cand[i].dm);
      fprintf(fp, "%f\t%d\t%d\t%d\t%d\n", hw_cand[i].snr.to_float(), (int)hw_cand[i].loc_img, (int)hw_cand[i].boxcar_width, (int)hw_cand[i].time, (int)hw_cand[i].dm);
    }
  }
  fclose(fp);
    
  fprintf(stdout, "INFO: DONE RESULT CHECK\n");
  fprintf(stdout, "INFO: Elapsed time of CPU code is %E seconds\n", cpu_elapsed_time);
  fprintf(stdout, "INFO: Elapsed time of kernel is %E seconds\n", kernel_elapsed_time);
    
  // Cleanup
  clReleaseMemObject(buffer_in);
  clReleaseMemObject(buffer_history_in);
  clReleaseMemObject(buffer_history_out);
  clReleaseMemObject(buffer_cand);
  
  free(in);
  free(history_in);
  free(sw_history_out);
  free(hw_history_out);
  free(sw_cand);
  free(hw_cand);
    
  clReleaseProgram(program);
  clReleaseKernel(kernel_boxcar);
  clReleaseKernel(kernel_stream);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  fprintf(stdout, "INFO: DONE ALL\n");
  
  return EXIT_SUCCESS;
}
