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

// Report structure for fine-grained flux costs from a kernel execution
struct KernelFluxReport {
    uint64_t cycle_cost = 0;
    uint64_t hw_in_cost = 0;  // Hamming weight of input data
    uint64_t hw_out_cost = 0; // Hamming weight of output data
};

// Forward declaration from api/vpu.h, assuming it defines VPU_Task
// This is to avoid circular dependency if vpu.h includes hal.h indirectly.
// Proper solution might involve a dedicated types header.
struct VPU_Task;

// The Kernel Library uses std::function to create a generic, extensible HAL.
// Kernels now return a flux report and take the VPU_Task by reference to access data.
// JIT-compiled kernels might be different (see Pillar4_Cerebellum) if they fully capture state.
using GenericKernel = std::function<KernelFluxReport(VPU_Task& task)>;
using KernelLibrary = std::map<std::string, GenericKernel>;

// Specialized SAXPY versions for JIT demonstration
void cpu_saxpy_sparse_specialized(float a, const std::vector<float>& x, std::vector<float>& y);
void cpu_saxpy_dense_specialized(float a, const std::vector<float>& x, std::vector<float>& y);

} // namespace HAL
} // namespace VPU
