#include "hal/hal.h"
#include <iostream>
#include <vector> // Ensure vector is included for std::vector parameters
#include <fftw3.h> // For FFTW functions

namespace VPU {
namespace HAL {

// --- SAXPY ---
void cpu_saxpy(float a, const std::vector<float>& x, std::vector<float>& y) {
    // FLUX-AWARE OPTIMIZATION:
    // If 'a' is zero, the operation is a no-op.
    // This simple check avoids potentially millions of operations.
    if (a == 0.0f) {
        std::cout << "    -> [HAL KERNEL] SAXPY Flux-Optimization triggered (alpha=0). Skipping computation." << std::endl;
        return;
    }
    std::cout << "    -> [HAL KERNEL] Executing SAXPY on CPU." << std::endl;
    for (size_t i = 0; i < x.size(); ++i) {
        y[i] = a * x[i] + y[i];
    }
}

// --- GEMM (Naive) ---
void cpu_gemm_naive(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int M, int N, int K) {
    std::cout << "    -> [HAL KERNEL] Executing Naive GEMM (Matrix-Matrix Multiply)." << std::endl;
    // Standard, highly inefficient triple-loop implementation. Serves as a baseline.
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < K; ++k) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// --- GEMM (Flux-Adaptive) ---
void cpu_gemm_flux_adaptive(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, int M, int N, int K) {
    std::cout << "    -> [HAL KERNEL] Executing Flux-Adaptive GEMM (Optimized for Sparsity)." << std::endl;
    // This is a conceptual kernel. A real implementation would convert to a sparse
    // format like CSR and only process non-zero elements.
    // We simulate its higher efficiency by just running the same naive code.
    // The VPU's decision is based on the *predicted* cost, not the actual kernel code here.
     for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < K; ++k) {
                 if (A[i * K + k] != 0 && B[k * N + j] != 0) { // Simulate sparsity check
                    sum += A[i * K + k] * B[k * N + j];
                }
            }
            C[i * N + j] = sum;
        }
    }
}


// --- FFT Implementations using FFTW3 ---
void cpu_fft_forward(const std::vector<double>& signal_in, std::vector<double>& complex_out_interleaved) {
    std::cout << "    -> [HAL KERNEL] Executing Actual FFTW3 Forward Transform (R2C)." << std::endl;
    if (signal_in.empty()) {
        complex_out_interleaved.clear();
        std::cerr << "Warning: cpu_fft_forward called with empty input signal." << std::endl;
        return;
    }
    int N = signal_in.size();

    double* fftw_in_real;
    fftw_complex* fftw_out_complex;
    fftw_plan plan_r2c;

    fftw_in_real = (double*)fftw_malloc(sizeof(double) * N);
    if (!fftw_in_real) {
        std::cerr << "FFTW3 Error: fftw_malloc for input failed in cpu_fft_forward." << std::endl;
        complex_out_interleaved.clear();
        return;
    }
    fftw_out_complex = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));
    if (!fftw_out_complex) {
        std::cerr << "FFTW3 Error: fftw_malloc for output failed in cpu_fft_forward." << std::endl;
        fftw_free(fftw_in_real);
        complex_out_interleaved.clear();
        return;
    }

    for (int i = 0; i < N; ++i) {
        fftw_in_real[i] = signal_in[i];
    }

    plan_r2c = fftw_plan_dft_r2c_1d(N, fftw_in_real, fftw_out_complex, FFTW_ESTIMATE);
    if (!plan_r2c) {
        std::cerr << "FFTW3 Error: fftw_plan_dft_r2c_1d failed in cpu_fft_forward." << std::endl;
        fftw_free(fftw_out_complex);
        fftw_free(fftw_in_real);
        complex_out_interleaved.clear();
        return;
    }

    fftw_execute(plan_r2c);

    complex_out_interleaved.resize((N / 2 + 1) * 2);
    for (int i = 0; i < (N / 2 + 1); ++i) {
        complex_out_interleaved[2 * i] = fftw_out_complex[i][0];
        complex_out_interleaved[2 * i + 1] = fftw_out_complex[i][1];
    }

    fftw_destroy_plan(plan_r2c);
    fftw_free(fftw_out_complex);
    fftw_free(fftw_in_real);
}

// --- Specialized SAXPY Stubs for JIT ---
// ... (Specialized SAXPY stubs remain unchanged here, they are already correct from previous steps)

void cpu_fft_inverse(const std::vector<double>& complex_in_interleaved, std::vector<double>& signal_out, int N_original_time_samples) {
    std::cout << "    -> [HAL KERNEL] Executing Actual FFTW3 Inverse Transform (C2R)." << std::endl;

    if (complex_in_interleaved.empty() || N_original_time_samples <= 0) {
        signal_out.clear();
        std::cerr << "Warning: cpu_fft_inverse called with empty input or invalid N_original_time_samples." << std::endl;
        return;
    }

    int N = N_original_time_samples;
    int num_complex_inputs = N / 2 + 1;

    if (complex_in_interleaved.size() != (size_t)num_complex_inputs * 2) {
        std::cerr << "FFTW3 Error: complex_in_interleaved size mismatch in cpu_fft_inverse. Expected "
                  << num_complex_inputs * 2 << " got " << complex_in_interleaved.size()
                  << " for N_original_time_samples=" << N << std::endl;
        signal_out.clear();
        return;
    }

    fftw_complex* fftw_in_complex;
    double* fftw_out_real;
    fftw_plan plan_c2r;

    fftw_in_complex = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * num_complex_inputs);
    if (!fftw_in_complex) {
        std::cerr << "FFTW3 Error: fftw_malloc for input failed in cpu_fft_inverse." << std::endl;
        signal_out.clear();
        return;
    }
    fftw_out_real = (double*)fftw_malloc(sizeof(double) * N);
    if (!fftw_out_real) {
        std::cerr << "FFTW3 Error: fftw_malloc for output failed in cpu_fft_inverse." << std::endl;
        fftw_free(fftw_in_complex);
        signal_out.clear();
        return;
    }

    for (int i = 0; i < num_complex_inputs; ++i) {
        fftw_in_complex[i][0] = complex_in_interleaved[2 * i];
        fftw_in_complex[i][1] = complex_in_interleaved[2 * i + 1];
    }

    plan_c2r = fftw_plan_dft_c2r_1d(N, fftw_in_complex, fftw_out_real, FFTW_ESTIMATE);
    if (!plan_c2r) {
        std::cerr << "FFTW3 Error: fftw_plan_dft_c2r_1d failed in cpu_fft_inverse." << std::endl;
        fftw_free(fftw_out_real);
        fftw_free(fftw_in_complex);
        signal_out.clear();
        return;
    }

    fftw_execute(plan_c2r);

    signal_out.resize(N);
    for (int i = 0; i < N; ++i) {
        signal_out[i] = fftw_out_real[i] / N; // Normalization
    }

    fftw_destroy_plan(plan_c2r);
    fftw_free(fftw_out_real);
    fftw_free(fftw_in_complex);
}

// --- Specialized SAXPY Stubs for JIT ---
void cpu_saxpy_sparse_specialized(float a, const std::vector<float>& x, std::vector<float>& y_mut) {
    std::cout << "    -> [HAL KERNEL] Executing JIT-selected CPU SAXPY (SPARSE SPECIALIZED STUB) for a=" << a << "." << std::endl;
    if (!y_mut.empty()) {
        float x_val = x.empty() ? 0.0f : x[0]; // Use x[0] if available, else 0
        y_mut[0] = y_mut[0] + (a * x_val) + 1.0f;
    }
}

void cpu_saxpy_dense_specialized(float a, const std::vector<float>& x, std::vector<float>& y_mut) {
    std::cout << "    -> [HAL KERNEL] Executing JIT-selected CPU SAXPY (DENSE SPECIALIZED STUB) for a=" << a << "." << std::endl;
    if (!y_mut.empty()) {
        float x_val = x.empty() ? 0.0f : x[0]; // Use x[0] if available, else 0
        y_mut[0] = y_mut[0] + (a * x_val) + 2.0f;
    }
}


} // namespace HAL
} // namespace VPU
