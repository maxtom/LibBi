/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_CUDA_RESAMPLER_RESAMPLERGPU_CUH
#define BI_CUDA_RESAMPLER_RESAMPLERGPU_CUH

#include "ResamplerKernel.cuh"
#include "../../math/temp_vector.hpp"
#include "../../misc/omp.hpp"

#include "thrust/fill.h"

template<class V1>
void bi::ResamplerGPU::permute(V1 as) {
  /* pre-condition */
  BI_ASSERT(V1::on_device);

  const int P = as.size();
  temp_gpu_vector<int>::type is(P);
  thrust::fill(is.begin(), is.end(), -1);

  dim3 Dg, Db;
  Db.x = bi::min(128, P);
  Dg.x = (P + Db.x - 1)/Db.x;

  kernelResamplerPrePermute<<<Dg,Db>>>(as.buf(), is.buf(), P);
  CUDA_CHECK;

  kernelResamplerPermute<<<Dg,Db>>>(as.buf(), is.buf(), P);
  CUDA_CHECK;
}

template<class V1, class M1>
void bi::ResamplerGPU::copy(const V1 as, M1 X) {
  /* pre-condition */
  BI_ASSERT(as.size() <= X.size1());
  BI_ASSERT(V1::on_device);
  BI_ASSERT(M1::on_device);

//  const int P = X.size1();
  const int P = as.size();
  const int N = X.size2();

  dim3 Dg, Db;
  Db.x = bi::min(128, P);
  Dg.x = (P + Db.x - 1)/Db.x;
  Db.y = (128 + Db.x - 1)/Db.x;
  Dg.y = (N + Db.y - 1)/Dg.y;

  kernelResamplerCopy<<<Dg,Db>>>(as.buf(), X);
  CUDA_CHECK;
}

#endif