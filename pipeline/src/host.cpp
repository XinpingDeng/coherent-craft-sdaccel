#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <chrono>

#include "host.h"
#include "krnl.h"

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"

// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"

//______________________________________________________________________________
////////MAIN FUNCTION//////////
int main(int argc, char **argv) {

  // Full size
  cl_int i;
  cl_int ntime       = 256;
  cl_int ndm         = 1024;
  cl_int nsmp_per_uv = MAX_SMP_PER_UV;
  cl_int ncu         = 2;
  cl_int fftsz       = MAX_FFTSZ;
  
  // Reduce size for emulation
  if (xcl::is_hw_emulation()) {
    ntime       = 2;
    ndm         = TILE_WIDTH_TRAN*ncu;
    nsmp_per_uv = MAX_SMP_PER_UV;
    fftsz       = MAX_FFTSZ;
  } else if (xcl::is_emulation()) {
    ntime       = 2;
    ndm         = TILE_WIDTH_TRAN*ncu;
    nsmp_per_uv = MAX_SMP_PER_UV;
    fftsz       = MAX_FFTSZ;
  }

  uint64_t nsmp_per_img = fftsz*fftsz;
  uint64_t ndata_coord  = nsmp_per_uv;
  uint64_t ndata_uv     = 2*ntime*ndm*nsmp_per_uv;
  uint64_t ndata_img    = 2*ntime*ndm*nsmp_per_img;
  
  uint64_t size_coord   = ndata_coord*sizeof(int);
  uint64_t size_uv      = ndata_uv*sizeof(real_t);
  uint64_t size_img     = ndata_img*sizeof(real_t);
  
  uint64_t ndata_uv_per_cu  = ndata_uv/ncu;
  uint64_t ndata_img_per_cu = ndata_img/ncu;
  uint64_t size_uv_per_cu   = ndata_uv_per_cu*sizeof(real_t);
  uint64_t size_img_per_cu  = ndata_img_per_cu*sizeof(real_t);

  cl_int nburst_per_uv   = nsmp_per_uv/NCMPLX_PER_BURST;
  cl_int nburst_dm       = ndm/NCMPLX_PER_BURST;
  cl_int nplane          = ndm*ntime;
  cl_int nburst_per_img  = msmp_per_img/ NCMPLX_PER_BURST;
  
  // I/O Data Vectors
  std::vector<int, aligned_allocator<int>> coord(ndata_coord);
  std::vector<real_t, aligned_allocator<real_t>> uv1(ndata_uv);
  std::vector<real_t, aligned_allocator<real_t>> uv2(ndata_uv);
  std::vector<real_t, aligned_allocator<real_t>> img(ndata_img);

  // Reset memory
  memset(coord, 0, size_coord);
  memset(coord, 0, size_uv);
  memset(coord, 0, size_uv);
  memset(coord, 0, size_img);
  
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // OpenCL Host Code Begins.
  cl_int err;

  // OpenCL objects
  cl::Device device;
  cl::Context context;
  cl::CommandQueue q;
  cl::Program program;
  
  auto binaryFile = argv[1];
  char krnl_name[MAX_LINE_LEN];
  
  // get_xil_devices() is a utility API which will find the xilinx
  // platforms and will return list of devices connected to Xilinx platform
  auto devices = xcl::get_xil_devices();

  // Selecting the first available Xilinx device
  device = devices[0];

  // Creating Context
  OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));

  // Creating Command Queue
  OCL_CHECK(err,
            q = cl::CommandQueue(context,
                                 device,
                                 CL_QUEUE_PROFILING_ENABLE |
                                 CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                                 &err));

  // read_binary_file() is a utility API which will load the binaryFile
  // and will return the pointer to file buffer.
  auto fileBuf = xcl::read_binary_file(binaryFile);

  cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
  devices.resize(1);

  // Creating Program
  OCL_CHECK(err, program = cl::Program(context, devices, bins, NULL, &err));

  // Creating Kernel
  std::string cu_id;
  std::string krnl_name_full;
  std::string krnl_tran_name  = "krnl_tran";
  std::string krnl_grid_name  = "krnl_grid";
  std::string krnl_fft_name   = "krnl_fft";
  std::string krnl_write_name = "krnl_write";
  std::vector<cl::Kernel> krnl_tran(ncu);
  std::vector<cl::Kernel> krnl_grid(ncu);
  std::vector<cl::Kernel> krnl_fft(ncu);
  std::vector<cl::Kernel> krnl_write(ncu);
  for(i = 0; i < ncu; i++){
    cu_id = std::to_string(i+1);

    krnl_name_full = krnl_tran_name + ":{" + krnl_tran_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_tran[i]  = cl::Kernel(program, krnl_name_full, &err));

    krnl_name_full = krnl_grid_name + ":{" + krnl_grid_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_grid[i]  = cl::Kernel(program, krnl_name_full, &err));

    krnl_name_full = krnl_fft_name + ":{" + krnl_fft_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_fft[i]   = cl::Kernel(program, krnl_name_full, &err));

    krnl_name_full = krnl_write_name + ":{" + krnl_write_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_write[i] = cl::Kernel(program, krnl_name_full, &err));
  }
  
  // Allocate Buffer in Global Memory
  // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
  // Device-to-host communication
  std::vector<cl::Buffer> buffer_coord(ncu);
  std::vector<cl::Buffer> buffer_uv1(ncu);
  std::vector<cl::Buffer> buffer_uv2(ncu);
  std::vector<cl::Buffer> buffer_out(ncu);
  uint64_t buffer_start;
  for(i = 0; i < ncu; i++){
    OCL_CHECK(err,
              cl::Buffer buffer_coord[i](context,
                                         CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                         size_coord,
                                         coord.data(),
                                         &err));
    
    buffer_start = i*ndata_uv_per_cu;
    OCL_CHECK(err,
              cl::Buffer buffer_uv1[i](context,
                                       CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       size_uv_per_cu,
                                       uv1.data()+buffer_start,
                                       &err));
    
    buffer_start = i*ndata_uv_per_cu;
    OCL_CHECK(err,
              cl::Buffer buffer_uv2[i](context,
                                       CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       size_uv_per_cu,
                                       uv2.data()+buffer_start,
                                       &err));
    
    buffer_start = i*ndata_img_per_cu;
    OCL_CHECK(err,
              cl::Buffer buffer_out[i](context,
                                       CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                       size_img_per_cu,
                                       out.data()+buffer_start,
                                       &err));
  }
  
  // Setting Kernel Arguments
  for(i = 0 ; i < ncu; i++){
    OCL_CHECK(err, err = krnl_tran[i].setArg(0, buffer_uv1[i]));
    OCL_CHECK(err, err = krnl_tran[i].setArg(1, buffer_uv2[i]));
    OCL_CHECK(err, err = krnl_tran[i].setArg(2, nburst_per_uv));
    OCL_CHECK(err, err = krnl_tran[i].setArg(3, ntime));
    OCL_CHECK(err, err = krnl_tran[i].setArg(4, nburst_dm));
    
    OCL_CHECK(err, err = krnl_grid[i].setArg(0, buffer_uv2[i]));
    OCL_CHECK(err, err = krnl_grid[i].setArg(1, buffer_coord[i]));
    OCL_CHECK(err, err = krnl_grid[i].setArg(3, nplane));
    OCL_CHECK(err, err = krnl_grid[i].setArg(4, nburst_per_uv));
    OCL_CHECK(err, err = krnl_grid[i].setArg(5, nburst_per_img));

    OCL_CHECK(err, err = krnl_write[i].setArg(0, nplane));
    OCL_CHECK(err, err = krnl_write[i].setArg(1, nburst_per_img));
    OCL_CHECK(err, err = krnl_write[i].setArg(3, buffer_out[i]));
  }
  
  // Copy input data to device global memory
  for(i = 0; i < ncu; i++){
    OCL_CHECK(
              err,
              err = q.enqueueMigrateMemObjects({buffer_uv1[i], buffer_coord[i]},
                                               0 /* 0 means from host*/));
  }
  
  // wait until all the data is moved over
  q.finish();
  
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // wall clock start
  auto kernel_start = std::chrono::high_resolution_clock::now();

  // v2
  for(i = 0; i < ncu; i++){
    OCL_CHECK(err, err = q.enqueueTask(krnl_tran[i]));
    OCL_CHECK(err, err = q.enqueueTask(krnl_grid[i]));
    OCL_CHECK(err, err = q.enqueueTask(krnl_fft[i]));
    OCL_CHECK(err, err = q.enqueueTask(krnl_write[i]));
  }
  q.finish();

  // wall clock end
  auto kernel_end = std::chrono::high_resolution_clock::now();


  std::cout << std::endl << "<<<<<< kernel duration >>>>>" << std::endl;
  print_time_diff(std::cout, kernel_start, kernel_end);
  std::cout << std::endl;
  std::cout << std::endl;
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


  // Copy Result from Device Global Memory to Host Local Memory
  for(i = 0; i < ncu; i++){
    OCL_CHECK(err,
              err = q.enqueueMigrateMemObjects({buffer_out[i]},
                                               CL_MIGRATE_MEM_OBJECT_HOST));
  }
  q.finish();

  // OpenCL Host Code Ends
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  return EXIT_SUCCESS;
}
