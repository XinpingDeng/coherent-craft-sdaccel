/*
******************************************************************************
** MAIN FUNCTION
******************************************************************************
*/

#include "util_sdaccel.h"
#include "grid.h"

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
  cl_int ndata1;
  cl_int ndata2;
  cl_int ndata3;
  cl_int ndm          = 1024;
  cl_int ntime_per_cu = 256;
  cl_int nuv_per_cu;

  if(is_hw_emulation()){
    ndm          = 2;
    ntime_per_cu = 2;
  }
  if(is_sw_emulation()){
    ndm          = 2;
    ntime_per_cu = 2;
  }
  nuv_per_cu = ntime_per_cu*ndm;

  ndata1 = 2*NSAMP_PER_UV_IN;
  ndata2 = 2*nuv_per_cu*NSAMP_PER_UV_IN;
  ndata3 = 2*nuv_per_cu*NSAMP_PER_UV_OUT;
  
  core_data_type *in = NULL;
  core_data_type *coordinate = NULL;
  core_data_type *sw_out = NULL;
  core_data_type *hw_out = NULL;
  cl_float *coordinate_float = NULL;
  
  in               = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(core_data_type));
  coordinate       = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  sw_out           = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata3*sizeof(core_data_type));
  hw_out           = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata3*sizeof(core_data_type));
  coordinate_float = (cl_float *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(cl_float));  
  
  fprintf(stdout, "INFO: %f MB memory used on host in total\n",
	  (ndata2 + ndata1 + 2*ndata3)*sizeof(core_data_type)/(1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device in total\n",
	  (ndata2 + ndata1 + ndata3)*sizeof(core_data_type)/(1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device for raw input\n",
	  ndata2*sizeof(core_data_type)/(1024.*1024.));  
  fprintf(stdout, "INFO: %f MB memory used on device for raw output\n",
	  ndata3*sizeof(core_data_type)/(1024.*1024.));  
  
  // Prepare input
  cl_uint i;
  srand(time(NULL));
  for(i = 0; i < ndata2; i++){
    in[i] = (core_data_type)(0.99*(rand()%DATA_RANGE));
  }
  read_coordinate("/data/FRIGG_2/Workspace/coherent-craft-sdaccel/grid/src/coordinate.txt", NSAMP_PER_UV_IN, coordinate_float);
  for(i = 0; i < ndata1; i++){
    coordinate[i] = (core_data_type)coordinate_float[i];
  }
  memset(sw_out, 0x00, ndata3*sizeof(core_data_type));
  memset(hw_out, 0x00, ndata3*sizeof(core_data_type));
  
  // Calculate on host
  cl_float cpu_elapsed_time;
  struct timespec host_start;
  struct timespec host_finish;
  clock_gettime(CLOCK_REALTIME, &host_start);
  fprintf(stdout, "HERE\n");
  grid(in, coordinate, sw_out, nuv_per_cu);
  fprintf(stdout, "INFO: DONE HOST EXECUTION\n");
  clock_gettime(CLOCK_REALTIME, &host_finish);
  cpu_elapsed_time = (host_finish.tv_sec - host_start.tv_sec) + (host_finish.tv_nsec - host_start.tv_nsec)/1.0E9L;
  fprintf(stdout, "HERE\n");
  
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
  OCL_CHECK(err, kernel = clCreateKernel(program, "knl_grid", &err));

  // Prepare device buffer
  cl_mem buffer_in;
  cl_mem buffer_coordinate;
  cl_mem buffer_out;
  cl_mem pt[3];

  OCL_CHECK(err, buffer_in         = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(core_data_type)*ndata2, in, &err));
  OCL_CHECK(err, buffer_coordinate = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(core_data_type)*ndata1, coordinate, &err));
  OCL_CHECK(err, buffer_out        = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(core_data_type)*ndata3, hw_out, &err));
  if (!(buffer_in&&
	buffer_coordinate&&
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
  pt[1] = buffer_coordinate;
  pt[2] = buffer_out;

  OCL_CHECK(err, err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_in));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_coordinate)); 
  OCL_CHECK(err, err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_out));
  OCL_CHECK(err, err = clSetKernelArg(kernel, 3, sizeof(cl_int), &nuv_per_cu));
  
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
  
  // Check the result
  /*
  for(i=0;i<ndata3;i++){
    if(sw_out[i] != hw_out[i]){
      fprintf(stderr, "ERROR: Test failed\n");
    }
  }
  fprintf(stdout, "INFO: DONE RESULT CHECK\n");
  */
  
  fprintf(stdout, "INFO: Elapsed time of CPU code is %E seconds\n", cpu_elapsed_time);
  fprintf(stdout, "INFO: Elapsed time of kernel is %E seconds\n", kernel_elapsed_time);
  
  // Cleanup
  clReleaseMemObject(buffer_in);
  clReleaseMemObject(buffer_coordinate);
  clReleaseMemObject(buffer_out);
  
  free(in);
  free(coordinate);
  free(sw_out);
  free(coordinate_float);
  
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  fprintf(stdout, "INFO: DONE ALL\n");
  
  return EXIT_SUCCESS;
}
