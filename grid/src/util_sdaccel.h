/*
******************************************************************************
** SDACCEL UTIL CODE HEADER FILE
******************************************************************************
*/

#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <stdbool.h>
#include <iostream>

cl_uint load_file_to_memory(const char *filename, char **result);
cl_device_id get_device_id(const char* target_device_name);

//OCL_CHECK doesn't work if call has templatized function call
#define OCL_CHECK(error, call)						\
  call;									\
  if (error != CL_SUCCESS) {						\
    fprintf(stderr, "ERROR: %s:%d Error calling " #call ", error code is: %d\n", \
	    __FILE__,__LINE__, error);					\
    exit(EXIT_FAILURE);							\
  }                                       

bool is_sw_emulation();
bool is_hw_emulation();
bool is_xpr_device(const char *device_name);
