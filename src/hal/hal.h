#pragma once

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <iostream>

namespace VPU {
namespace HAL {

// A collection of flux-aware, optimized kernels for various operations.
// In a real system, these would contain SIMD intrinsics or GPU-specific code.

// SAXPY: y = a*x + y. A core BLAS level-1 operation.
void cpu_saxpy(float a, const std::vector<float>& x, std::vector<float>& y);

// GEMM: C = alpha*A*B. A core BLAS level-3 operation.
void cpu_gemm_naive(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int M, int N, int K);

// A conceptual kernel that is more efficient for sparse matrices.
void cpu_gemm_flux_adaptive(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int M, int N, int K);

// Placeholder for a high-performance FFT kernel.
void cpu_fft_forward(const std::vector<double>& in, std::vector<double>& out);
void cpu_fft_inverse(const std::vector<double>& in, std::vector<double>& out, int N_original_time_samples);

// The Kernel Library uses std::function to create a generic, extensible HAL.
using GenericKernel = std::function<void()>;
using KernelLibrary = std::map<std::string, GenericKernel>;

// Specialized SAXPY versions for JIT demonstration
void cpu_saxpy_sparse_specialized(float a, const std::vector<float>& x, std::vector<float>& y);
void cpu_saxpy_dense_specialized(float a, const std::vector<float>& x, std::vector<float>& y);

} // namespace HAL
} // namespace VPU
