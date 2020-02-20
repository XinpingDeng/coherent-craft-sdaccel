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
template<typename T>
void print_time_diff(std::ostream& out, T prior, T latter)
{
  namespace sc = std::chrono;
  auto diff = sc::duration_cast<sc::milliseconds>(latter - prior).count();

#if 1
  auto const msecs = diff % 1000;
  diff /= 1000;
  auto const secs = diff % 60; 
  diff /= 60; 
  auto const mins = diff % 60; 
  diff /= 60; 
  auto const hours = diff % 24; 
  diff /= 24; 
  auto const days = diff;
#endif

#if 0
  unsigned int msecs = diff % 1000;
  diff /= 1000;
  unsigned int secs = diff % 60; 
  diff /= 60; 
  unsigned int mins = diff % 60; 
  diff /= 60; 
  unsigned int hours = diff % 24; 
  diff /= 24; 
  unsigned int days = diff;
#endif

  bool printed_earlier = false;
  if (days >= 1) {
    printed_earlier = true;
    out << days << (1 != days ? " days" : " day") << ' ';
  }   
  if (printed_earlier || hours >= 1) {
    printed_earlier = true;
    out << hours << (1 != hours ? " hours" : " hour") << ' ';
  }   
  if (printed_earlier || mins >= 1) {
    printed_earlier = true;
    out << mins << (1 != mins ? " minutes" : " minute") << ' ';
  }   
  if (printed_earlier || secs >= 1) {
    printed_earlier = true;
    out << secs << (1 != secs ? " seconds" : " second") << ' ';
  }   
  if (printed_earlier || msecs >= 1) {
    printed_earlier = true;
    out << std::dec << msecs << (1 != msecs ? " milliseconds" : " millisecond");
  }   
}

//______________________________________________________________________________
////////MAIN FUNCTION//////////
int main(int argc, char **argv) {

  // Full size
  cl_int i;
  cl_int ndm;
  cl_int nsmp_per_img;
  cl_int ntime       = 256;
  cl_int ncu         = 2;
  cl_int ndm_per_cu  = 512; // distribute DM to different CU, 1024 in total
  cl_int nsmp_per_uv = MAX_SMP_PER_UV;
  cl_int fftsz       = MAX_FFTSZ;
  
  ntime       = 2;
  ndm_per_cu  = TILE_WIDTH_TRAN;  // minimum on TILE_WIDTH_TRAN
  nsmp_per_uv = MAX_SMP_PER_UV;
  fftsz       = MAX_FFTSZ;
    
  // Reduce size for emulation
  if (xcl::is_hw_emulation()) {
    ntime       = 2;
    ndm_per_cu  = TILE_WIDTH_TRAN;  // minimum on TILE_WIDTH_TRAN
    nsmp_per_uv = MAX_SMP_PER_UV;
    fftsz       = MAX_FFTSZ;
  } else if (xcl::is_emulation()) {
    ntime       = 2;
    ndm_per_cu  = TILE_WIDTH_TRAN;
    nsmp_per_uv = MAX_SMP_PER_UV;
    fftsz       = MAX_FFTSZ;
  }
  ndm          = ndm_per_cu*ncu;
  nsmp_per_img = fftsz*fftsz;
  
  uint64_t ndata_coord = nsmp_per_uv; // Coord is the same for all CUs
  uint64_t ndata_uv    = 2*ntime*ndm*nsmp_per_uv;  // Complex
  uint64_t ndata_img   = 2*ntime*ndm*nsmp_per_img; // Complex
  
  uint64_t size_coord  = ndata_coord*sizeof(coord_t);
  uint64_t size_uv     = ndata_uv*sizeof(real_t);
  uint64_t size_img    = ndata_img*sizeof(real_t);
  
  uint64_t ndata_uv_per_cu  = 2*ntime*ndm_per_cu*nsmp_per_uv;
  uint64_t ndata_img_per_cu = 2*ntime*ndm_per_cu*nsmp_per_img;
  uint64_t size_uv_per_cu   = ndata_uv_per_cu*sizeof(real_t);
  uint64_t size_img_per_cu  = ndata_img_per_cu*sizeof(real_t);

  cl_int nplane_per_cu    = ndm_per_cu*ntime;
  cl_int nburst_dm_per_cu = ndm_per_cu/NCMPLX_PER_BURST;
  cl_int nburst_per_uv    = nsmp_per_uv/NCMPLX_PER_BURST;
  cl_int nburst_per_img   = nsmp_per_img/ NCMPLX_PER_BURST;
  
  // I/O Data Vectors
  std::vector<coord_t, aligned_allocator<coord_t>> coord(ndata_coord);
  std::vector<real_t,  aligned_allocator<real_t>> uv1(ndata_uv);
  std::vector<real_t,  aligned_allocator<real_t>> uv2(ndata_uv);
  std::vector<real_t,  aligned_allocator<real_t>> img(ndata_img);
  std::cout << std::endl << "<<<<<< memory used on device is " <<
    (2*size_uv+size_coord+size_img)/2014.0/1024.0 <<
    " MB >>>>>" <<
    std::endl;
  
  // Reset memory
  std::fill(coord.begin(), coord.end(), 0);
  std::fill(uv1.begin(), uv1.end(), 0);
  std::fill(uv2.begin(), uv2.end(), 0);
  std::fill(img.begin(), img.end(), 0);

  // Fill random numbers
  std::generate(uv1.begin(), uv1.end(), std::rand);
  
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // OpenCL Host Code Begins.
  cl_int err;

  // OpenCL objects
  cl::Device device;
  cl::Context context;
  cl::CommandQueue q;
  cl::Program program;
  
  auto binaryFile = argv[1];
  
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
    OCL_CHECK(err, krnl_tran[i]  = cl::Kernel(program, krnl_name_full.c_str(), &err));
    //std::cout << krnl_name_full <<std::endl;
    
    krnl_name_full = krnl_grid_name + ":{" + krnl_grid_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_grid[i]  = cl::Kernel(program, krnl_name_full.c_str(), &err));
    //std::cout << krnl_name_full <<std::endl;
    
    krnl_name_full = krnl_fft_name + ":{" + krnl_fft_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_fft[i]   = cl::Kernel(program, krnl_name_full.c_str(), &err));
    //std::cout << krnl_name_full <<std::endl;
    
    krnl_name_full = krnl_write_name + ":{" + krnl_write_name + "_" + cu_id + "}";
    OCL_CHECK(err, krnl_write[i] = cl::Kernel(program, krnl_name_full.c_str(), &err));
    //std::cout << krnl_name_full <<std::endl;    
  }

  // Allocate Buffer in Global Memory
  // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
  // Device-to-host communication
  std::vector<cl::Buffer> buffer_coord(ncu);
  std::vector<cl::Buffer> buffer_uv1(ncu);
  std::vector<cl::Buffer> buffer_uv2(ncu);
  std::vector<cl::Buffer> buffer_img(ncu);
  uint64_t buffer_start;
  
  for(i = 0; i < ncu; i++){
    OCL_CHECK(err,
              buffer_coord[i] =
              cl::Buffer(context,
                         CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                         size_coord,
                         coord.data(),
                         &err));
    
    buffer_start = i*ndata_uv_per_cu;
    OCL_CHECK(err,
              buffer_uv1[i] =
              cl::Buffer(context,
                         CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                         size_uv_per_cu,
                         uv1.data()+buffer_start,
                         &err));
    
    buffer_start = i*ndata_uv_per_cu;
    OCL_CHECK(err,
              buffer_uv2[i] =
              cl::Buffer(context,
                         CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                         size_uv_per_cu,
                         uv2.data()+buffer_start,
                         &err));
    
    buffer_start = i*ndata_img_per_cu;
    OCL_CHECK(err,
              buffer_img[i] =
              cl::Buffer(context,
                         CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                         size_img_per_cu,
                         img.data()+buffer_start,
                         &err));
  }
  
  // Setting Kernel Arguments
  for(i = 0 ; i < ncu; i++){
    OCL_CHECK(err, err = krnl_tran[i].setArg(0, buffer_uv1[i]));
    OCL_CHECK(err, err = krnl_tran[i].setArg(1, buffer_uv2[i]));
    OCL_CHECK(err, err = krnl_tran[i].setArg(2, nburst_per_uv));
    OCL_CHECK(err, err = krnl_tran[i].setArg(3, ntime));
    OCL_CHECK(err, err = krnl_tran[i].setArg(4, nburst_dm_per_cu));
    
    OCL_CHECK(err, err = krnl_grid[i].setArg(0, buffer_uv2[i]));
    OCL_CHECK(err, err = krnl_grid[i].setArg(1, buffer_coord[i]));
    OCL_CHECK(err, err = krnl_grid[i].setArg(3, nplane_per_cu));
    OCL_CHECK(err, err = krnl_grid[i].setArg(4, nburst_per_uv));
    OCL_CHECK(err, err = krnl_grid[i].setArg(5, nburst_per_img));

    OCL_CHECK(err, err = krnl_fft[i].setArg(0, nplane_per_cu));
    OCL_CHECK(err, err = krnl_fft[i].setArg(1, nburst_per_img));
    
    OCL_CHECK(err, err = krnl_write[i].setArg(0, nplane_per_cu));
    OCL_CHECK(err, err = krnl_write[i].setArg(1, nburst_per_img));
    OCL_CHECK(err, err = krnl_write[i].setArg(3, buffer_img[i]));
  }
  
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
              err = q.enqueueMigrateMemObjects({buffer_img[i]},
                                               CL_MIGRATE_MEM_OBJECT_HOST));
  }
  q.finish();

  // OpenCL Host Code Ends
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  return EXIT_SUCCESS;
}
