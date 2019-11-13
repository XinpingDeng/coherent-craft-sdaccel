/*
******************************************************************************
** MAIN FUNCTION
******************************************************************************
*/

#include "util_sdaccel.h"
#include "prepare.h"

int main(int argc, char* argv[])
{
  /* Prepare the data arraies */

  cl_uint ndata1;
  cl_uint ndata2;
  cl_uint ndata3;
  
  ndata1 = 2 * NSAMP_PER_TIME;
  ndata2 = 2 * NTIME_PER_CU * NSAMP_PER_TIME;
  
  core_data_type *in_pol1 = NULL;
  core_data_type *in_pol2 = NULL;
  core_data_type *sw_out = NULL;
  core_data_type *hw_out = NULL;
  core_data_type *cal_pol1 = NULL;
  core_data_type *cal_pol2 = NULL;
  core_data_type *sky = NULL;
  core_data_type *sw_average_pol1 = NULL;
  core_data_type *sw_average_pol2 = NULL;
  core_data_type *hw_average_pol1 = NULL;
  core_data_type *hw_average_pol2 = NULL;

  in_pol1  = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(core_data_type));
  in_pol2  = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(core_data_type));
  sw_out   = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(core_data_type));
  hw_out   = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata2*sizeof(core_data_type));
  
  cal_pol1 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  cal_pol2 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  sky      = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  sw_average_pol1 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  sw_average_pol2 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  hw_average_pol1 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  hw_average_pol2 = (core_data_type *)aligned_alloc(MEM_ALIGNMENT, ndata1*sizeof(core_data_type));
  
  fprintf(stdout, "INFO: %f MB memory used on host in total\n",
	  (4*ndata2 + 7*ndata1)*sizeof(core_data_type)/(1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device in total\n",
	  (3*ndata2 + 5*ndata1)*sizeof(core_data_type)/(1024.*1024.));
  fprintf(stdout, "INFO: %f MB memory used on device for raw input\n",
	  2*ndata2*sizeof(core_data_type)/(1024.*1024.));  
  fprintf(stdout, "INFO: %f MB memory used on device for raw output\n",
	  ndata2*sizeof(core_data_type)/(1024.*1024.));  
  
  /* Do the calculation */
  srand(time(NULL));
  cl_uint i;
  cl_float elapsed_time;
  struct timespec start_host;
  struct timespec stop_host;
    
  /* Prepare input and output */
  for(i = 0; i < ndata2; i++){
    in_pol1[i] = (core_data_type)(rand()%DATA_RANGE);
    in_pol2[i] = (core_data_type)(rand()%DATA_RANGE);
  }
  
  for(i = 0; i < ndata1; i++){
    cal_pol1[i] = (core_data_type)(rand()%DATA_RANGE);
    cal_pol2[i] = (core_data_type)(rand()%DATA_RANGE);
    sky[i] = (core_data_type)(rand()%DATA_RANGE);
  }
  
  /* Calculate on host */
  clock_gettime(CLOCK_REALTIME, &start_host);
  prepare(in_pol1,
	  in_pol2,
	  cal_pol1,
	  cal_pol2,
	  sky,
	  sw_out,
	  sw_average_pol1,
	  sw_average_pol2);
  fprintf(stdout, "\nDONE HOST EXECUTION\n\n");
  clock_gettime(CLOCK_REALTIME, &stop_host);
  elapsed_time = (stop_host.tv_sec - start_host.tv_sec) + (stop_host.tv_nsec - start_host.tv_nsec)/1.0E9L;
  printf("INFO: elapsed_time of one loop is %E seconds\n", elapsed_time);
  
  
  /* Do the calculation on card */
  if (argc != 3) {
    printf("Usage: %s xclbin\n", argv[0]);
    return EXIT_FAILURE;
  }	
  const char* target_device_name = argv[2];
  
  cl_device_id device_id;            
  cl_context context;                
  cl_command_queue commands;         
  cl_program program;                
  cl_kernel kernel_prepare;       
  
  cl_mem buffer_in_pol1;
  cl_mem buffer_in_pol2;                 
  cl_mem buffer_cal_pol1;
  cl_mem buffer_cal_pol2;
  cl_mem buffer_sky;
  cl_mem buffer_out;
  cl_mem buffer_average_pol1;
  cl_mem buffer_average_pol2;
  cl_mem pt[8];
  cl_int err;
  
  device_id = get_device_id(target_device_name);
  context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
  if (!context) {
    printf("Error: Failed to create a compute context!\n");
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  commands = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
  if (!commands) {
    printf("Error: Failed to create a command commands!\n");
    printf("Error: code %i\n",err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  
  cl_int status;
  unsigned char *kernelbinary;
  char *xclbin = argv[1];
  printf("INFO: loading xclbin %s\n", xclbin);
  cl_uint n_i0 = load_file_to_memory(xclbin, (char **) &kernelbinary);
  if (n_i0 < 0) {
    printf("failed to load kernel from xclbin: %s\n", xclbin);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  
  size_t n0 = n_i0;
  program = clCreateProgramWithBinary(context, 1, &device_id, &n0,
				      (const unsigned char **) &kernelbinary, &status, &err);
  free(kernelbinary);
  
  if ((!program) || (err!=CL_SUCCESS)) {
    printf("Error: Failed to create compute program from binary %d!\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS) {
    size_t len;
    char buffer[2048];

    printf("Error: Failed to build program executable!\n");
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    printf("%s\n", buffer);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  
  kernel_prepare = clCreateKernel(program, "knl_prepare", &err);
  if (!kernel_prepare || err != CL_SUCCESS) {
    printf("Error: Failed to create compute kernel_vector_add!\n");
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  
  buffer_in_pol1 = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata2, in_pol1, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - in_pol1" << err<< std::endl;
  }
  buffer_in_pol2 = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata2, in_pol2, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - in_pol2" << err << std::endl;
  }  
  buffer_sky = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata1, sky, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - sky" << err << std::endl;
  }
  buffer_cal_pol1 = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata1, cal_pol1, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - cal_pol1" << err << std::endl;
  }
  buffer_cal_pol2 = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata1, cal_pol2, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - cal_pol2" << err << std::endl;
  }
  buffer_out = clCreateBuffer(context,  CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata2, hw_out, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - out" << err << std::endl;
  }  
  buffer_average_pol1 = clCreateBuffer(context,  CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata1, hw_average_pol1, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - average_pol1" << err << std::endl;
  }  
  buffer_average_pol2 = clCreateBuffer(context,  CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,  sizeof(core_data_type)*ndata1, hw_average_pol2, &err);
  if (err != CL_SUCCESS) {
    std::cout << "Return code for clCreateBuffer - average_pol2" << err << std::endl;
  }
  
  if (!(buffer_in_pol1&&
	buffer_in_pol2&&
	buffer_out&&
	buffer_sky&&
	buffer_cal_pol1&&
	buffer_cal_pol2&&
	buffer_average_pol1&&
	buffer_average_pol2
	)) {
    printf("Error: Failed to allocate device memory!\n");
    printf("Test failed\n");
    return EXIT_FAILURE;
  }

  pt[0] = buffer_in_pol1;
  pt[1] = buffer_in_pol2;
  pt[2] = buffer_cal_pol1;
  pt[3] = buffer_cal_pol2;
  pt[4] = buffer_sky;
  pt[5] = buffer_out;
  pt[6] = buffer_average_pol1;
  pt[7] = buffer_average_pol2;

  // To use multiple banks, this has to be before any enqueue options (e.g., clEnqueueMigrateMemObjects)
  err = 0;
  err |= clSetKernelArg(kernel_prepare, 0, sizeof(cl_mem), &buffer_in_pol1);
  err |= clSetKernelArg(kernel_prepare, 1, sizeof(cl_mem), &buffer_in_pol2); 
  err |= clSetKernelArg(kernel_prepare, 2, sizeof(cl_mem), &buffer_cal_pol1);
  err |= clSetKernelArg(kernel_prepare, 3, sizeof(cl_mem), &buffer_cal_pol2); 
  err |= clSetKernelArg(kernel_prepare, 4, sizeof(cl_mem), &buffer_sky);
  err |= clSetKernelArg(kernel_prepare, 5, sizeof(cl_mem), &buffer_out); 
  err |= clSetKernelArg(kernel_prepare, 6, sizeof(cl_mem), &buffer_average_pol1);
  err |= clSetKernelArg(kernel_prepare, 7, sizeof(cl_mem), &buffer_average_pol2);  
  if (err != CL_SUCCESS) {
    printf("Error: Failed to set kernel_prepare arguments! %d\n", err);
    printf("Test failed\n");
 
  }

  fprintf(stdout, "\nDONE SETUP KERNEL\n\n");
  err = clEnqueueMigrateMemObjects(commands,(cl_uint)5,pt, 0 ,0,NULL, NULL);
  if (err != CL_SUCCESS) {
    printf("Error: Failed to set kernel_prepare arguments! %d\n", err);
    printf("Test failed\n");
  }
  err = clFinish(commands);
  if (err != CL_SUCCESS) {
    printf("Error: Failed to finish the commands: %d!\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "\nDONE MEMCPY FROM HOST TO KERNEL\n\n");
  
  struct timespec start_device;
  struct timespec stop_device;
  clock_gettime(CLOCK_REALTIME, &start_device);
  err = clEnqueueTask(commands, kernel_prepare, 0, NULL, NULL);
  if (err) {
    printf("Error: Failed to execute kernel! %d\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  err = clFinish(commands);
  if (err != CL_SUCCESS) {
    printf("Error: Failed to finish the commands: %d!\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "\nDONE KERNEL EXECUTION\n\n");
  clock_gettime(CLOCK_REALTIME, &stop_device);
  elapsed_time = (stop_device.tv_sec - start_device.tv_sec) + (stop_device.tv_nsec - start_device.tv_nsec)/1.0E9L;
  fprintf(stdout, "Elapsed time of kernel is %E seconds \n", elapsed_time);

  err = 0;
  err |= clEnqueueMigrateMemObjects(commands,(cl_uint)3,&pt[5], CL_MIGRATE_MEM_OBJECT_HOST,0,NULL, NULL);  
  if (err != CL_SUCCESS) {
    printf("Error: Failed to write to source array: %d!\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }

  err = clFinish(commands);
  if (err != CL_SUCCESS) {
    printf("Error: Failed to finish the commands: %d!\n", err);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  fprintf(stdout, "\nDONE MEMCPY FROM KERNEL TO HOST\n\n");
  
  double res = 1.0E-2;
  ndata3 = 0;
  for(i=0;i<ndata1;i++){
    if(fabs((sw_average_pol1[i]-hw_average_pol1[i])/sw_average_pol1[i])>res){
      std::cout << "Mismatch on average_pol1: " <<i << '\t' << sw_average_pol1[i]<< '\t'<< hw_average_pol1[i] << '\n';
      ndata3++;
    }
  }
  fprintf(stdout, "\n%.0f %% of AVERAGE_POL1 is outside %.0f %% range\n\n", (float)ndata3/ndata1, 100*res);

  ndata3 = 0;
  for(i=0;i<ndata1;i++){
    if(fabs((sw_average_pol2[i]-hw_average_pol2[i])/sw_average_pol2[i])>res){
      std::cout << "Mismatch on average_pol2: " <<i << '\t' << sw_average_pol2[i]<< '\t'<< hw_average_pol2[i] << '\n';
      ndata3++;
    }
  }
  fprintf(stdout, "\n%.0f %% of AVERAGE_POL2 is outside %.0f %% range\n\n", (float)ndata3/ndata1, 100*res);
  
  for(i=0;i<ndata2;i++){
    ndata3 = 0;
    if(fabs((sw_out[i]-hw_out[i])/sw_out[i])>res){
      std::cout << "Mismatch on out: " <<i << '\t'<< sw_out[i]<< '\t'<< hw_out[i] << '\t' << ((sw_out[i]-hw_out[i])/sw_out[i])<< '\n';
      ndata3++;
    }
  }
  fprintf(stdout, "\n%.0f %% of OUT is outside %.0f %% range\n\n", (float)ndata3/ndata2, 100*res);

  std::cout << "Mismatch on out: " <<i << '\t'<< sw_out[i]<< '\t'<< hw_out[i] << '\t' << ((sw_out[i]-hw_out[i])/sw_out[i])<< '\n';
  
  /* Free memory */
  clReleaseMemObject(buffer_in_pol1);
  clReleaseMemObject(buffer_in_pol2);
  clReleaseMemObject(buffer_cal_pol1);
  clReleaseMemObject(buffer_cal_pol2);
  clReleaseMemObject(buffer_sky);
  clReleaseMemObject(buffer_out);
  clReleaseMemObject(buffer_average_pol1);
  clReleaseMemObject(buffer_average_pol2);
  
  free(in_pol1);
  free(in_pol2);
  free(sw_out);
  free(hw_out);
  free(cal_pol1);
  free(cal_pol2);
  free(sky);
  free(sw_average_pol1);
  free(sw_average_pol2);
  free(hw_average_pol1);
  free(hw_average_pol2);
  
  clReleaseProgram(program);
  clReleaseKernel(kernel_prepare);
  clReleaseCommandQueue(commands);
  clReleaseContext(context);

  fprintf(stdout, "\nDONE ALL\n\n");
  return EXIT_SUCCESS;
}
