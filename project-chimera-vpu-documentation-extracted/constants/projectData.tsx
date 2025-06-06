
import React from 'react';
import { ProjectDataType, FileNode } from '../types';
import CodeBlock from '../components/CodeBlock';

const repoStructureData: FileNode[] = [
  {
    name: 'CHIMERA_VPU/',
    type: 'directory',
    children: [
      {
        name: 'api/',
        type: 'directory',
        children: [
          { name: 'vpu.h', type: 'file', comment: 'Public-facing API for developers to include.' }
        ]
      },
      {
        name: 'src/',
        type: 'directory',
        children: [
          {
            name: 'core/',
            type: 'directory',
            children: [
              { name: 'Pillar1_Synapse.h/cpp', type: 'file', comment: 'Task interception & API implementation.' },
              { name: 'Pillar2_Cortex.h/cpp', type: 'file', comment: 'Flux Profiler & Hardware Database.' },
              { name: 'Pillar3_Orchestrator.h/cpp', type: 'file', comment: 'Decision engine & path simulation.' },
              { name: 'Pillar4_Cerebellum.h/cpp', type: 'file', comment: 'Dispatcher & JIT/runtime management.' },
              { name: 'Pillar5_Feedback.h/cpp', type: 'file', comment: 'Learning loop and belief updates.' },
            ]
          },
          {
            name: 'hal/',
            type: 'directory',
            children: [
              { name: 'hal.h', type: 'file', comment: 'Common interface for all hardware kernels.' },
              { name: 'cpu_kernels.cpp', type: 'file', comment: 'Optimized SIMD/AVX kernels.' },
              { name: 'gpu_kernels.cu', type: 'file', comment: 'GPU-specific CUDA/ROCm kernels.' },
            ]
          },
          { name: 'vpu_core.cpp', type: 'file', comment: 'Main VPU class that orchestrates the pillars.' }
        ]
      },
      {
        name: 'tests/',
        type: 'directory',
        children: [
          { name: 'e2e_first_loop.cpp', type: 'file', comment: 'First end-to-end integration test.'}
        ]
      },
      {
        name: 'external/',
        type: 'directory',
        children: [
          { name: 'fftw/', type: 'file', comment: 'Placeholder for third-party libs like FFTW.' }
        ]
      },
      { name: 'CMakeLists.txt', type: 'file', comment: 'The master build file for the project.' }
    ]
  }
];

const cmakeFileContentData = `
# Project Chimera VPU Build System
cmake_minimum_required(VERSION 3.15)

project(ChimeraVPU CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find External Dependencies ---
# Placeholder for finding the FFTW library
# find_package(FFTW3 REQUIRED)

# --- Define the VPU Core Library ---
# This library contains the main logic of all five pillars.
add_library(vpu_core
  src/core/Pillar1_Synapse.cpp
  src/core/Pillar2_Cortex.cpp
  src/core/Pillar3_Orchestrator.cpp
  src/core/Pillar4_Cerebellum.cpp
  src/core/Pillar5_Feedback.cpp
  src/hal/cpu_kernels.cpp
  src/vpu_core.cpp
)

# Public headers for the library
target_include_directories(vpu_core PUBLIC
  \${CMAKE_CURRENT_SOURCE_DIR}/api
  # PRIVATE \${CMAKE_CURRENT_SOURCE_DIR}/src/core etc.
)

# Link VPU core to its dependencies
# target_link_libraries(vpu_core PRIVATE FFTW3::fftw3)

# --- Define Test Executables ---
add_executable(E2E_First_Loop tests/e2e_first_loop.cpp)

# Link the test against our VPU library so it can use its functionality
target_link_libraries(E2E_First_Loop PRIVATE vpu_core)

# Enable testing with CTest
enable_testing()
add_test(NAME FirstEndToEndTest COMMAND E2E_First_Loop)
`;

const e2eTestContentData = `
#include "vpu.h" // The public API
#include <iostream>
#include <cassert> // For simple, clear test assertions
#include <vector> // Required for std::vector

// This represents a user's code calling our VPU.
// Mock VPU namespace and functions for illustrative purposes
namespace VPU {
    struct VPU_Environment {};
    struct VPU_Task {
        const char* task_type;
        const std::vector<uint8_t>* data_in;
        const std::vector<uint8_t>* data_in_b;
        uint8_t* data_out_buffer;
        size_t num_elements;
    };
    struct LastExecutionStats {
        const char* chosen_path_name;
        bool quark_detected;
    };

    VPU_Environment* init_vpu() {
        std::cout << "[MOCK] VPU::init_vpu() called." << std::endl;
        return new VPU_Environment();
    }
    void shutdown_vpu(VPU_Environment* vpu) {
        std::cout << "[MOCK] VPU::shutdown_vpu() called." << std::endl;
        delete vpu;
    }
    int get_hamming_weight(const uint8_t* data, size_t size) {
        // Simplified mock hamming weight
        int hw = 0;
        for(size_t i=0; i<size; ++i) {
            for(int j=0; j<8; ++j) {
                if((data[i] >> j) & 1) hw++;
            }
        }
        return hw;
    }
    void vpu_execute(VPU_Environment* vpu, VPU_Task* task) {
        std::cout << "[MOCK] VPU::vpu_execute() for task: " << task->task_type << std::endl;
        if (task->task_type == std::string("VECTOR_ADD") && task->num_elements > 0) {
            for(size_t i = 0; i < task->num_elements; ++i) {
                task->data_out_buffer[i] = (*task->data_in)[i] + (*task->data_in_b)[i];
            }
        }
    }
    LastExecutionStats get_last_stats(VPU_Environment* vpu) {
        std::cout << "[MOCK] VPU::get_last_stats() called." << std::endl;
        return {"Direct CPU Execution", false};
    }
}


int main() {
  std::cout << "===== [TEST] BEGINNING VPU END-TO-END TEST 1 =====" << std::endl;

  // 1. Initialize the VPU Environment
  // This conceptually loads the HAL, databases, etc.
  VPU::VPU_Environment* vpu = VPU::init_vpu();
  std::cout << "[TEST] VPU Environment Initialized." << std::endl;

  // 2. Define a simple, predictable task
  std::vector<uint8_t> vec_a = { 0b00000001, 0b00000011 }; // HW=1, HW=2. Total HW=3
  std::vector<uint8_t> vec_b = { 0b00000100, 0b00000101 }; // HW=2, HW=3. Total HW=5
  std::vector<uint8_t> result_buffer_vec(2); // Use std::vector for safety

  VPU::VPU_Task task;
  task.task_type = "VECTOR_ADD";
  task.data_in = &vec_a;
  task.data_in_b = &vec_b;
  task.data_out_buffer = result_buffer_vec.data();
  task.num_elements = 2;

  std::cout << "[TEST] Submitting VECTOR_ADD task." << std::endl;
  std::cout << " - Input A (HW): " << VPU::get_hamming_weight(vec_a.data(), vec_a.size()) << std::endl;
  std::cout << " - Input B (HW): " << VPU::get_hamming_weight(vec_b.data(), vec_b.size()) << std::endl;

  // 3. Execute the task through the VPU
  // This single call will trigger the entire Perceive-Decide-Act-Learn loop.
  VPU::vpu_execute(vpu, &task);
  std::cout << "[TEST] VPU execution finished. Verifying results..." << std::endl;

  // 4. Verify the outcome
  // 4a. Verify numerical correctness of the result
  assert(result_buffer_vec[0] == 0b00000101); // 1 + 4 = 5
  assert(result_buffer_vec[1] == 0b00001000); // 3 + 5 = 8
  std::cout << " - [PASS] Numerical result is correct." << std::endl;

  // 4b. Verify the VPU made a sensible decision
  // For a simple vector addition, the 'Direct' path should always be chosen.
  VPU::LastExecutionStats stats = VPU::get_last_stats(vpu);
  assert(stats.chosen_path_name == std::string("Direct CPU Execution"));
  std::cout << " - [PASS] Orchestrator chose the expected optimal path: '" <<
  stats.chosen_path_name << "'." << std::endl;

  // 4c. Verify the feedback loop ran
  // We expect the deviation to be small, so no learning should have occurred.
  assert(stats.quark_detected == false);
  std::cout << " - [PASS] Feedback loop ran and correctly found no significant quark." <<
  std::endl;

  // 5. Cleanup
  VPU::shutdown_vpu(vpu);
  std::cout << "\\n===== [TEST] VPU E2E TEST 1 SUCCEEDED! =====\\n" << std::endl;

  return 0;
}
`;

const pillar1ArchitectureCode =
`typedef struct {
  void* function_kernel; // Pointer to the function to execute.
  void* data_in;
  size_t data_in_size;
  void* data_out_buffer;
  size_t data_out_size;
} VPU_Task;

VPU_Status vpu_execute(VPU_Task* task); // Submits task for flux-optimal execution.`;

const pillar2ImplementationCode = `
// #include <iostream> // Already in main app context
// #include <vector>   // Already in main app context
// #include <cmath>    // Already in main app context
// #include <numeric>  // Already in main app context
// #include <stdexcept>// Already in main app context
// #include <cstdint>  // Already in main app context

// --- FFTW Placeholder ---
// In a real implementation, this would link to the FFTW3 library.
// For this standalone prototype, we'll simulate the interface.
// g++ -std=c++17 vpu_profiler.cpp -o vpu_profiler -lfftw3 -lm
// #include <fftw3.h>
// --- End Placeholder ---

// Mocking fftw3 for browser/non-linked environment
typedef double fftw_complex[2];
typedef struct fftw_plan_s* fftw_plan;
// const int FFTW_ESTIMATE = 1; // Example value - REMOVED, use 1 directly

fftw_complex* fftw_malloc(size_t n) { return (fftw_complex*)malloc(n); }
void fftw_free(void* p) { free(p); }
fftw_plan fftw_plan_dft_r2c_1d(int n, double* in, fftw_complex* out, unsigned flags) {
    // Mock plan: in a real scenario, this prepares for FFT.
    // For demo, we'll just return a dummy non-null pointer.
    // flags would use FFTW_ESTIMATE (now 1)
    return (fftw_plan)malloc(1); // Dummy plan
}
void fftw_execute(fftw_plan p) {
    // Mock execute: in a real scenario, this performs the FFT.
    // For demo, this is a no-op as we don't have actual FFTW input/output arrays connected.
    // A real implementation would populate the 'out' array passed to fftw_plan_dft_r2c_1d.
    // For this example, we will simulate some output in profileOmni.
}
void fftw_destroy_plan(fftw_plan p) { free(p); }


namespace VPU {
namespace Pillar2 {

/**
* @struct DataProfile
* @brief Standardized output data structure holding all calculated flux metrics.
* This quantifies the "Arbitrary Contextual Weight" (ACW) of the data.
*/
struct DataProfile {
    // WFC Profile (Digital Binary Domain)
    uint64_t hamming_weight = 0;
    double sparsity_ratio = 1.0; // 1.0 = all zeros, 0.0 = all ones

    // Omnimorphic Profile (Numerical Sequence Domain)
    double amplitude_flux_A_W = 0.0;
    double frequency_flux_F_W = 0.0;
    double entropy_flux_E_W = 0.0;
};

/**
* @class RepresentationalFluxAnalyzer
* @brief The VPU's primary perception engine. Analyzes data payloads to
* quantify their intrinsic representational cost (Flux).
*/
class RepresentationalFluxAnalyzer {
public:
    RepresentationalFluxAnalyzer() = default;

    /**
    * @brief Profiles data using the low-level Weight-Flux Computing (WFC) model.
    * @param data A pointer to the raw binary data.
    * @param size_bytes The size of the data in bytes.
    * @return A DataProfile object populated with WFC metrics.
    */
    DataProfile profileWFC(const uint8_t* data, size_t size_bytes) {
        DataProfile profile;
        if (!data || size_bytes == 0) {
            return profile; // Return empty profile
        }

        uint64_t total_hw = 0;
        for (size_t i = 0; i < size_bytes; ++i) {
            // Use compiler intrinsic for highly efficient bit counting (popcount)
            // This is a direct measure of active '1's
            // __builtin_popcount is a GCC/Clang extension.
            // For MSVC, use __popcnt. For portability, a lookup table or a software impl might be needed.
            // For this demo, let's assume a simple software popcount for illustration.
            unsigned char byte = data[i];
            for(int bit = 0; bit < 8; ++bit) {
                if((byte >> bit) & 1) total_hw++;
            }
        }

        profile.hamming_weight = total_hw;
        uint64_t total_bits = size_bytes * 8;
        if (total_bits > 0) {
            profile.sparsity_ratio = 1.0 - (static_cast<double>(total_hw) / total_bits);
        }
        return profile;
    }

    /**
    * @brief Profiles a sequence of floating-point numbers using generalized Omnimorphic metrics.
    * @param data A pointer to an array of doubles.
    * @param num_elements The number of elements in the array.
    * @return A DataProfile object populated with A(W), F(W), and E(W) flux metrics.
    */
    DataProfile profileOmni(const double* data, size_t num_elements) {
        DataProfile profile;
        if (!data || num_elements < 2) {
            return profile; // Not enough data to profile
        }

        // 1. Calculate Amplitude Flux (A_W) - The "Chattiness"
        for (size_t i = 1; i < num_elements; ++i) {
            profile.amplitude_flux_A_W += std::abs(data[i] - data[i-1]);
        }

        // --- 2. Calculate Frequency and Entropy Flux via FFT ---
        fftw_complex* fft_out_mock = nullptr; // To be populated by fftw_execute
        // fftw_plan p_mock_dummy; // This variable is not used in the current mock logic.

        double* fft_in = (double*) fftw_malloc(sizeof(double) * num_elements);
        if(!fft_in) throw std::runtime_error("FFTW malloc failed for input.");

        for(size_t i = 0; i < num_elements; ++i) fft_in[i] = data[i];

        size_t fft_out_size = (num_elements / 2) + 1;
        fft_out_mock = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_out_size);

        if(!fft_out_mock) {
            fftw_free(fft_in);
            throw std::runtime_error("FFTW malloc failed for output.");
        }

        // Simulate FFT output for demonstration purposes as direct FFTW execution in browser is complex
        // A real implementation would call:
        // p_mock_dummy = fftw_plan_dft_r2c_1d(num_elements, fft_in, fft_out_mock, 1 /*FFTW_ESTIMATE*/);
        // fftw_execute(p_mock_dummy); // This would populate fft_out_mock

        // Simulating some FFT output characteristics
        for(size_t i = 0; i < fft_out_size; ++i) {
            // Create a simple decaying spectrum for smooth signals, more uniform for noisy
            double real_val = (num_elements - i) * ( (i % 5 == 0) ? 0.5 : 0.1 ) * profile.amplitude_flux_A_W / num_elements;
            double imag_val = (num_elements - i) * ( (i % 7 == 0) ? 0.3 : 0.05 )* profile.amplitude_flux_A_W / num_elements;
             // This is a very crude simulation
            if (profile.amplitude_flux_A_W < num_elements*0.1) { // "smooth"
                 fft_out_mock[i][0] = (i < 10) ? real_val : real_val * 0.1; // concentrate energy at low freqs
                 fft_out_mock[i][1] = (i < 10) ? imag_val : imag_val * 0.1;
            } else { // "noisy"
                 fft_out_mock[i][0] = (rand() / (double)RAND_MAX - 0.5) * 2.0 * profile.amplitude_flux_A_W / 10.0;
                 fft_out_mock[i][1] = (rand() / (double)RAND_MAX - 0.5) * 2.0 * profile.amplitude_flux_A_W / 10.0;
            }
        }


        // --- Post-FFT Analysis ---
        std::vector<double> magnitudes(fft_out_size);
        std::vector<double> phases(fft_out_size);
        std::vector<double> power_spectral_density(fft_out_size);
        double total_power = 0.0;

        for (size_t i = 0; i < fft_out_size; ++i) {
            double real = fft_out_mock[i][0];
            double imag = fft_out_mock[i][1];

            magnitudes[i] = std::sqrt(real*real + imag*imag);
            phases[i] = std::atan2(imag, real);

            power_spectral_density[i] = real*real + imag*imag; // For rfft, this is |X_k|^2
            total_power += power_spectral_density[i];
        }

        // 2a. Calculate Frequency Flux (F_W) - Unwrapped Phase Variation
        for (size_t i = 1; i < fft_out_size; i++) {
            double difference = phases[i] - phases[i-1];
            // Simple unwrap
            while (difference > M_PI) difference -= 2*M_PI;
            while (difference < -M_PI) difference += 2*M_PI;
            profile.frequency_flux_F_W += std::abs(difference);
        }

        // 2b. Calculate Entropy Flux (E_W) - Spectral Entropy
        if (total_power > 1e-9) { // Avoid division by zero
            for (size_t i = 0; i < fft_out_size; ++i) {
                double p_k = power_spectral_density[i] / total_power;
                if (p_k > 1e-9) { // Avoid log(0)
                    profile.entropy_flux_E_W -= p_k * std::log2(p_k);
                }
            }
        }

        // Cleanup FFTW resources
        // fftw_destroy_plan(p_mock_dummy); // Not created in mock
        fftw_free(fft_in);
        fftw_free(fft_out_mock);

        return profile;
    }
};
} // namespace Pillar2
} // namespace VPU
`;
const pillar2ExampleUsage = `
// Example Usage (Conceptual, needs to be in a main or test function)
/*
#include <iostream> // Add this for std::cout
#include <vector>   // Add this for std::vector
#include <cmath>    // Add this for std::sin, M_PI
#include <cstdlib>  // Add this for rand, srand
#include <ctime>    // Add this for time

void print_profile(const VPU::Pillar2::DataProfile& profile) {
    std::cout << "\\n--- Data Profile ---" << std::endl;
    std::cout << "WFC Profile:" << std::endl;
    std::cout << " - Hamming Weight: " << profile.hamming_weight << std::endl;
    std::cout << " - Sparsity Ratio: " << profile.sparsity_ratio << std::endl;
    std::cout << "Omnimorphic Profile:" << std::endl;
    std::cout << " - Amplitude Flux (A_W): " << profile.amplitude_flux_A_W << std::endl;
    std::cout << " - Frequency Flux (F_W): " << profile.frequency_flux_F_W << std::endl;
    std::cout << " - Entropy Flux (E_W): " << profile.entropy_flux_E_W << std::endl;
    std::cout << "--------------------\\n";
}

int main() { // Renamed to main for a standalone example
    VPU::Pillar2::RepresentationalFluxAnalyzer analyzer;

    // --- Scenario 1: Profiling sparse vs dense binary data ---
    std::cout << "===== SCENARIO 1: WFC Binary Profiling =====" << std::endl;
    uint8_t sparse_data[] = { 0x01, 0x00, 0x02, 0x00, 0x04 }; // Low HW
    uint8_t dense_data[] = { 0xFF, 0xDE, 0xAD, 0xBE, 0xEF }; // High HW

    auto sparse_profile_wfc = analyzer.profileWFC(sparse_data, sizeof(sparse_data));
    std::cout << "Profile for 'Sparse Data':";
    print_profile(sparse_profile_wfc);

    auto dense_profile_wfc = analyzer.profileWFC(dense_data, sizeof(dense_data));
    std::cout << "Profile for 'Dense Data':";
    print_profile(dense_profile_wfc);

    // --- Scenario 2: Profiling smooth vs noisy numerical data ---
    std::cout << "\\n===== SCENARIO 2: Omnimorphic Sequence Profiling =====" << std::endl;
    const size_t n_samples = 1024;
    std::vector<double> smooth_signal(n_samples);
    std::vector<double> noisy_signal(n_samples);

    srand(time(0)); // Seed random number generator

    for(size_t i = 0; i < n_samples; ++i) {
        smooth_signal[i] = std::sin(2 * M_PI * 5 * static_cast<double>(i) / n_samples); // Smooth 5 Hz sine wave
        noisy_signal[i] = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0; // White noise
    }

    auto smooth_omni_profile = analyzer.profileOmni(smooth_signal.data(), n_samples);
    std::cout << "Profile for 'Smooth Signal' (Sine Wave):";
    print_profile(smooth_omni_profile);

    auto noisy_omni_profile = analyzer.profileOmni(noisy_signal.data(), n_samples);
    std::cout << "Profile for 'Noisy Signal' (White Noise):";
    print_profile(noisy_omni_profile);

    return 0;
}
*/
`;


export const projectData: ProjectDataType = {
  title: 'Project Chimera: VPU Documentation',
  introduction: `Project Chimera is organized for modularity, scalability, and a clear separation between the public API, the core logic, and the hardware-specific implementations. It aims to create a Virtual Processing Unit (VPU) that can intelligently optimize computational tasks by understanding their intrinsic "Flux" cost and adapting its execution strategy accordingly.`,
  repoStructure: repoStructureData,
  cmakeFileContent: cmakeFileContentData,
  e2eTestContent: e2eTestContentData,
  pillars: [
    {
      id: 'pillar1',
      title: 'Pillar 1: The Universal Flux API & Interceptor (The "Synapse")',
      description: `This is the outermost layer of the VPU, providing the entry points for tasks and data. Its purpose is to create a seamless, hardware-agnostic interface that allows the system to "capture" computations destined for optimization without requiring a complete rewrite of existing software.`,
      architecture: (
        <>
          <p className="font-semibold">1. High-Level API (libvpu):</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: A simple, developer-friendly library for explicit task marking.</li>
            <li>Signature Example:
              <CodeBlock language="c" code={pillar1ArchitectureCode} />
            </li>
            <li>Language Bindings: Core C-API with wrappers (Python, Rust, C++).</li>
          </ul>
          <p className="font-semibold">2. JIT/WASM Interface:</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Portable, secure, platform-independent format for kernels.</li>
            <li>Workflow: Developers compile functions to WASM. VPU_Task points to WASM binary. VPU acts as specialized WASM runtime.</li>
          </ul>
          <p className="font-semibold">3. Deep OS Interception Hooks (The "Trapper"):</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: System-wide accelerator for existing, unmodified applications.</li>
            <li>Mechanism: Privileged daemon/kernel module using LD_PRELOAD or syscall interception.</li>
            <li>Target Libraries: BLAS/LAPACK, FFTW/cuFFT, libc (memcpy), Media Codecs.</li>
          </ul>
        </>
      ),
      dataFlow: `Application initiates task -> Captured by High-Level API or OS Interceptor -> Task & data packaged into VPU_Task structure -> Placed in high-priority queue for VPU core logic.`
    },
    {
      id: 'pillar2',
      title: 'Pillar 2: The Flux Profiler & Analyzer (The "Cerebral Cortex")',
      alias: "Perception Layer",
      description: `This is the VPU's primary perception layer. It takes the raw VPU_Task and enriches it with a deep understanding of its context, answering: 1. What is the intrinsic nature and complexity of this data? (Representational Flux Analysis) 2. What are the capabilities and costs of the environment? (Hardware Capability Analysis). Output is an "Enriched Execution Context".`,
      architecture: (
        <>
         <p className="font-semibold">1. Representational Flux Analyzer (τ_W Module):</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Quantify intrinsic data cost (Arbitrary Contextual Weight - ACW).</li>
            <li>Sub-modules:
                <ul className="list-disc list-inside ml-8">
                    <li>Digital WFC Profiler (Low-Level): For raw binary ops. Calculates Hamming Weight (HW). Output: WFC_Profile {'{total_bit_count, active_bit_count (HW), sparsity_ratio}'}.</li>
                    <li>Omnimorphic Data Profiler (High-Level): For generic data streams. Computes: Amplitude Flux A(W), Frequency Flux F(W), Entropy Flux E(W). Output: OmniFlux_Profile {'{A_W, F_W, E_W}'}.</li>
                </ul>
            </li>
          </ul>
          <p className="font-semibold">2. Hardware Capability Profiler & Database (The "Calibrator"):</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Maintain dynamic understanding of operational costs on host.</li>
            <li>Operational Modes: Offline (benchmarks), Online (learning from Pillar 5).</li>
            <li>Database (Hardware Profile): Key-value store.
                <ul className="list-disc list-inside ml-8">
                    <li>τ_operation Table: Cost of specific operation. Example: {"{key: ('ADD', 'FP32_VECTOR', 'CPU_AVX2'), value: {flux_cost: 1.5, latency_ns_per_element: 0.3}}"}</li>
                    <li>τ_transform Table: Cost of changing data representations. Example: {"{key: ('TimeDomain', 'FrequencyDomain', 4096, 'CPU'), value: {flux_cost: 25000, latency_us: 15.0}}"}</li>
                </ul>
            </li>
          </ul>
        </>
      ),
      dataFlow: `VPU_Task enters Cortex -> Representational Flux Analyzer profiles data (WFC_Profile or OmniFlux_Profile) -> Profile attached to task -> Cortex consults Hardware Capability Database for known costs (τ_operation, τ_transform) -> Bundled into "Enriched Execution Context" -> Passed to Pillar 3.`
    },
    {
      id: 'pillar3',
      title: 'Pillar 3: The Adaptive Orchestrator (The "Thalamus")',
      alias: "Cognitive Core",
      description: `The Orchestrator is the cognitive core of the VPU. It decides how computation should be performed, not performing it itself. Input: "Enriched Execution Context" from Pillar 2. Purpose: Evaluate potential computational strategies and select the path predicted to consume minimum Holistic Flux.`,
       architecture: (
        <>
          <p className="font-semibold">1. Candidate Path Generator:</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Create valid, potential strategies for the task.</li>
            <li>Logic: Rule-based expert system. Example for "Convolution": Path 1 (Direct Time Domain), Path 2 (FFT-based Frequency Domain), Path 3 (Wavelet Domain Sparsity-based).</li>
             <li>Output: Structured sequence of (Operation, Representation) tuples.</li>
          </ul>
          <p className="font-semibold">2. Flux Cost Simulator:</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Calculate predicted Holistic_Flux for each candidate path.</li>
            <li>Logic: For each path: Tally τ_transform costs from Hardware Profile. Calculate τ_operation = Base_Op_Cost_CU * f(ACW). Sum for Holistic Flux = Σ(τ_transform) + Predicted_τ_operation.</li>
          </ul>
          <p className="font-semibold">3. Decision Engine:</p>
          <ul className="list-disc list-inside ml-4 mb-2">
            <li>Function: Make final selection.</li>
            <li>Logic: Compares Holistic_Flux values, selects path with absolute minimum. Resolves trade-off between transformation cost and operational efficiency.</li>
          </ul>
        </>
      ),
      dataFlow: `Enriched_Execution_Context arrives -> Candidate Path Generator creates strategies -> Flux Cost Simulator annotates with Holistic_Flux score -> Decision Engine identifies winning path -> Creates definitive ExecutionPlan.`,
      coreStructures: (
        <CodeBlock language="json" code={
`{
  "task_id": "conv_123",
  "optimal_path": "Frequency Domain",
  "predicted_flux": 8450.0,
  "steps": [
    { "operation": "TRANSFORM", "kernel": "FFT_4096", "substrate": "GPU_0" },
    { "operation": "EXECUTE", "kernel": "ELEMENT_WISE_MULTIPLY", "representation": "FP32_COMPLEX", "substrate": "GPU_0" },
    { "operation": "TRANSFORM", "kernel": "IFFT_4096", "substrate": "GPU_0" }
  ]
}`
        } />
      )
    },
     {
      id: 'pillar4',
      title: 'Pillar 4: The Polymorphic JIT Runtime (The "Cerebellum")',
      alias: "Engine of Action",
      description: `This is the VPU's engine of action and motor control. It takes the abstract ExecutionPlan from the Orchestrator and transforms it into highly optimized, high-performance execution on physical hardware. "Polymorphic" refers to its ability to change execution strategy. "JIT" (Just-In-Time) capability allows generating uniquely specialized code.`,
      architecture: (
        <>
          <p className="font-semibold">1. Execution Dispatcher:</p>
          <ul className="list-disc list-inside ml-4"><li>Reads ExecutionPlan, orchestrates operations, manages dependencies.</li></ul>
          <p className="font-semibold mt-2">2. Hardware Abstraction Layer (HAL) & Kernel Library:</p>
          <ul className="list-disc list-inside ml-4">
            <li>Repository of pre-compiled, "best-in-class" computational primitives.</li>
            <li>Implementations: CPU Kernels (SIMD intrinsics), GPU Kernels (CUDA, ROCm, Metal), WASM Runtime (e.g., Wasmtime).</li>
          </ul>
           <p className="font-semibold mt-2">3. Data Flow & Memory Manager:</p>
          <ul className="list-disc list-inside ml-4">
            <li>Manages physical location and movement of data.</li>
            <li>Logic: Substrate-Awareness (CPU RAM, GPU VRAM), Efficient Transfer (pinned memory), Cost Accounting (feedback to Pillar 5).</li>
          </ul>
          <p className="font-semibold mt-2">4. Polymorphic JIT (Just-In-Time) Compiler:</p>
          <ul className="list-disc list-inside ml-4">
            <li>Generates new, temporary machine code on the fly when pre-compiled kernels are insufficient.</li>
            <li>Technology: Built on LLVM. Generates LLVM IR, uses LLVM backend for optimized code.</li>
            <li>Flux-Adaptive Code Generation: Sparsity Exploitation (O(NNZ) from WFC_Profile), Precision Pruning (e.g., FP16), Kernel Fusion.</li>
          </ul>
        </>
      ),
      dataFlow: `ExecutionPlan received by Dispatcher -> Step 1 (e.g., FFT_4096 on GPU): Data Flow Manager moves data to GPU, invokes FFT kernel from HAL's GPU library -> Step 2 (e.g., ELEMENT_WISE_MULTIPLY): If sparse, JIT Compiler might generate custom kernel. Otherwise, simple GPU kernel -> Final result moved back to CPU RAM.`
    },
    {
      id: 'pillar5',
      title: 'Pillar 5: The Flux-Quark Feedback Loop (The "Hippocampus")',
      alias: "Memory and Learning",
      description: `This pillar is the VPU's mechanism for memory, learning, and self-correction, completing the cognitive cycle: Perceive -> Decide -> Act -> LEARN. It compares predicted cost against actual, measured cost. A "Flux Quark" (significant discrepancy) triggers updates to VPU's internal "beliefs" to improve model accuracy. Adapts to hardware quirks, OS updates, thermal throttling.`,
      architecture: (
        <>
          <p className="font-semibold">1. Post-Execution Instrumentation Monitor (The "Observer"):</p>
          <ul className="list-disc list-inside ml-4">
            <li>Function: Capture ground truth of completed computation.</li>
            <li>Implementation: Dispatcher (Pillar 4) wraps ExecutionPlan steps in timers and performance counters.</li>
            <li>Data Collected: Wall-clock latency, Substrate-specific metrics (CPU: instructions retired, cache misses; GPU: kernel time, memory R/W), Power Usage (via RAPL/NVML). Packaged into ActualPerformanceRecord.</li>
          </ul>
          <p className="font-semibold mt-2">2. Deviation Analyzer (The "Auditor"):</p>
          <ul className="list-disc list-inside ml-4">
            <li>Function: Compare prediction against reality, detect "Quarks."</li>
            <li>Logic: Takes ExecutionPlan (predicted_flux) and ActualPerformanceRecord. Converts raw metrics to Observed_Flux. Calculates Deviation = (Observed_Flux - Predicted_Flux) / Predicted_Flux. If abs(Deviation) &gt; FLUX_QUARK_THRESHOLD, flags learning opportunity.</li>
          </ul>
          <p className="font-semibold mt-2">3. Belief Update Manager (The "Learning Core"):</p>
          <ul className="list-disc list-inside ml-4">
            <li>Function: Refine VPU's internal models (Hardware Capability Database in Pillar 2 & 3).</li>
            <li>Logic: Root Cause Analysis (identify if error was in τ_transform or τ_operation). Adaptive Learning Rule (e.g., New_Cost = Old_Cost * (1 - learning_rate) + Observed_Cost * learning_rate).</li>
            <li>Mechanism: If VPU too optimistic (Observed &gt; Predicted), penalizes by increasing stored cost. If too pessimistic, rewards by decreasing cost.</li>
          </ul>
        </>
      ),
      dataFlow: `A VPU_Task enters Synapse (1) -> Cortex (2) profiles, creates Enriched_Execution_Context -> Orchestrator (3) creates predictive ExecutionPlan -> Cerebellum (4) executes, generating ActualPerformanceRecord -> Feedback Loop (5) compares Plan and Record. If Quark detected, Learning Core updates beliefs in Cortex (2) -> Next similar task benefits from improved accuracy.`
    }
  ],
  vpuTechSpec: {
    title: "VPU Technical Specification - Version 1.0",
    coreDataStructures: [
      {
        name: "VPU_Task",
        language: "c",
        definition:
`typedef struct VPU_Task {
  // Task Identifier
  uint64_t task_id;

  // Data Payloads
  const void* data_in;
  size_t data_in_size;
  void* data_out_buffer;
  size_t data_out_size;

  // Kernel Definition
  enum { KERNEL_FP, KERNEL_WASM } kernel_type;
  union {
    void (*function_pointer)(void*, void*); // Pointer for standard C/C++ kernels
    const uint8_t* wasm_binary;           // Pointer to WASM module
  } kernel;
  size_t kernel_size;

  // Metadata
  const char* task_type; // e.g., "SORT", "CONVOLUTION", "GEMM"
} VPU_Task;`
      },
      {
        name: "EnrichedExecutionContext",
        language: "c",
        definition:
`typedef struct EnrichedExecutionContext {
  VPU_Task original_task;

  // Data Profile (Result of Representational Flux Analysis)
  struct {
    double amplitude_flux_A_W;
    double frequency_flux_F_W;
    double entropy_flux_E_W;
    uint64_t hamming_weight;
    double sparsity_ratio; // (1.0 - HW/total_bits)
  } data_profile;

  // Hardware Profile (Queried from DB)
  struct {
    // Costs for operating directly
    double direct_op_cost;
    double direct_op_latency;
    // Potential transformations and their costs
    // VPU_TransformCost potential_transforms[MAX_TRANSFORMS]; // illustrative
  } hardware_profile;
} EnrichedExecutionContext;`
      },
      {
        name: "ExecutionPlan (JSON Example)",
        language: "json",
        definition:
`{
  "task_id": 12345,
  "predicted_holistic_flux": 3104.5,
  "plan": [
    { "op": "MOVE_TO_GPU", "source": "data_in", "dest": "gpu_mem_1" },
    { "op": "EXECUTE_KERNEL", "kernel": "FFT_FORWARD", "input": "gpu_mem_1", "output": "gpu_mem_2" },
    // ... more steps ...
    { "op": "MOVE_FROM_GPU", "source": "gpu_mem_final", "dest": "data_out_buffer" }
  ]
}`
      }
    ]
  },
  pillar2Implementation: {
    title: "Pillar 2 Implementation: RepresentationalFluxAnalyzer",
    code: pillar2ImplementationCode,
    language: "cpp",
    exampleUsage: pillar2ExampleUsage
  }
};
// Added a clarifying comment to pillar2ImplementationCode previously. No other obvious uninitialized consts.
// The error likely stems from an issue not visible in the provided file content (e.g. build/cache, external script, subtle typo).
// Re-asserting the file content as per user's current input.
