# Project Chimera: VPU Documentation

Project Chimera is organized for modularity, scalability, and a clear separation between the public API, the core logic, and the hardware-specific implementations. It aims to create a Virtual Processing Unit (VPU) that can intelligently optimize computational tasks by understanding their intrinsic "Flux" cost and adapting its execution strategy accordingly.

## Repository Structure

- `CHIMERA_VPU/` (directory):
  - `api/` (directory):
    - `vpu.h` (file): Public-facing API for developers to include.
  - `src/` (directory):
    - `core/` (directory):
      - `Pillar1_Synapse.h/cpp` (file): Task interception & API implementation.
      - `Pillar2_Cortex.h/cpp` (file): Flux Profiler & Hardware Database.
      - `Pillar3_Orchestrator.h/cpp` (file): Decision engine & path simulation.
      - `Pillar4_Cerebellum.h/cpp` (file): Dispatcher & JIT/runtime management.
      - `Pillar5_Feedback.h/cpp` (file): Learning loop and belief updates.
    - `hal/` (directory):
      - `hal.h` (file): Common interface for all hardware kernels.
      - `cpu_kernels.cpp` (file): Optimized SIMD/AVX kernels.
      - `gpu_kernels.cu` (file): GPU-specific CUDA/ROCm kernels.
    - `vpu_core.cpp` (file): Main VPU class that orchestrates the pillars.
  - `tests/` (directory):
    - `e2e_full_loop.cpp` (file): Main end-to-end integration test.
  - `external/` (directory):
    - `fftw/` (file): Placeholder for third-party libraries like FFTW.
  - `CMakeLists.txt` (file): The master build file for the project.

## CMakeLists.txt Content

```cmake
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
  ${CMAKE_CURRENT_SOURCE_DIR}/api
  # PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core etc.
)

# Link VPU core to its dependencies
# target_link_libraries(vpu_core PRIVATE FFTW3::fftw3) # Actual CMakeLists.txt links FFTW3

# --- Define Test Executables ---
# The following is a simplified representation. The actual CMakeLists.txt
# includes additional dependencies like httplib, nlohmann_json, and PkgConfig for FFTW3.
add_executable(e2e_full_loop tests/e2e_full_loop.cpp)

# Link the test against our VPU library so it can use its functionality
target_link_libraries(e2e_full_loop PRIVATE vpu_core)

# Enable testing with CTest
enable_testing()
# The actual CMakeLists.txt also defines a DgmLoopTest.
add_test(NAME EndToEndLearningTest COMMAND e2e_full_loop)
```
## End-to-End Test Content (tests/e2e_full_loop.cpp)

**Note:** The C++ code below is an illustrative example of how a user might interact with a *mocked* version of the VPU API. This example is for conceptual understanding of a test case. The actual VPU API structures (`VPU_Task`, `VPU_Environment`) are defined in `api/vpu.h` and detailed in the "VPU Technical Specification - Version 1.0" section. The mock structures used in this specific test example may differ.

```cpp
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
```

## Building and Running the VPU

This section provides instructions on how to build the Project Chimera VPU and run the provided end-to-end test.

### Prerequisites

Before you begin, ensure you have the following installed on your system:
- **CMake:** Version 3.15 or higher. CMake is used to manage the build process.
- **C++ Compiler:** A compiler that supports C++17 (e.g., GCC, Clang, MSVC).
- **FFTW3 Library:** The Fastest Fourier Transform in the West library.
    - On Debian/Ubuntu: `sudo apt-get install libfftw3-dev`
    - On Fedora: `sudo dnf install fftw-devel`
    - On macOS (using Homebrew): `brew install fftw`

### Building the Project

The project uses CMake to generate build files for your specific platform and compiler.

1.  **Create a build directory:**
    It's good practice to create a separate directory for building, to keep the source tree clean.
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project using CMake:**
    From within the `build` directory, run CMake to configure the project and generate build files. This will detect your compiler and find necessary libraries.
    ```bash
    cmake ..
    ```
    If CMake has trouble finding FFTW3, you might need to specify its location using `CMAKE_PREFIX_PATH` or by setting `FFTW3_INCLUDE_DIRS` and `FFTW3_LIBRARIES` environment variables or CMake cache variables.

3.  **Compile the project:**
    After CMake has successfully configured the project, compile it using your build tool.
    - If you're using Makefiles (common on Linux/macOS):
      ```bash
      make
      ```
    - Alternatively, you can use the CMake build command (platform-agnostic):
      ```bash
      cmake --build .
      ```
    This will build the `vpu_core` library and the test executables (`e2e_full_loop` and `dgm_loop_test`) inside the `build` directory.

### Running the Tests

The primary end-to-end test is `e2e_full_loop`.

1.  **Navigate to the build directory:**
    If you're not already there, `cd` into your `build` directory.
    ```bash
    cd /path/to/project/build
    ```
    (Replace `/path/to/project/` with the actual path to the ChimeraVPU project root).

2.  **Run the `e2e_full_loop` test directly:**
    ```bash
    ./e2e_full_loop
    ```
    You should see output indicating the test progress and a success message at the end, similar to the "End-to-End Test Content (tests/e2e_full_loop.cpp)" section.

3.  **Run tests using CTest (Optional):**
    CMake also generates CTest configurations. You can run all registered tests using:
    ```bash
    ctest
    ```
    This will execute `EndToEndLearningTest` (which runs `e2e_full_loop`) and `DgmLoopTest` (which runs `dgm_loop_test`).

## VPU Architecture Pillars

### Pillar 1: The Universal Flux API & Interceptor (The "Synapse")

This is the outermost layer of the VPU, providing the entry points for tasks and data. Its purpose is to create a seamless, hardware-agnostic interface that allows the system to "capture" computations destined for optimization without requiring a complete rewrite of existing software.

#### Architecture

##### 1. High-Level API (libvpu):
- Function: A simple, developer-friendly library for explicit task marking.
- Signature Example:
  ```c
typedef struct VPU_Task {
  // Task Identifier
  uint64_t task_id;      // Unique identifier for the task
  std::string task_type; // e.g., "CONVOLUTION", "GEMM", "SAXPY", or a more generic "USER_DEFINED_KERNEL"

  // Kernel Definition
  enum class KernelType {
      FUNCTION_POINTER,
      WASM_BINARY
  };
  KernelType kernel_type;

  union Kernel {
      // Signature: (input_a, input_b, output, num_elements)
      // This is a simplified signature; a real system might need more flexible kernel signatures.
      void (*function_pointer)(const void* data_in_a, const void* data_in_b, void* data_out, size_t num_elements);
      const uint8_t* wasm_binary; // Pointer to WASM module binary data

      Kernel() : function_pointer(nullptr) {} // Default constructor for union
  } kernel;

  size_t kernel_size; // Optional: Size of the WASM binary, if kernel_type is WASM_BINARY

  // Data payload
  const void* data_in_a;
  const void* data_in_b;
  void* data_out;
  size_t num_elements; // Relevant for array/vector operations

  // Default constructor to initialize members
  VPU_Task() : task_id(0), kernel_type(KernelType::FUNCTION_POINTER), kernel_size(0),
               data_in_a(nullptr), data_in_b(nullptr), data_out(nullptr), num_elements(0) {}
} VPU_Task;

// VPU_Status vpu_execute(VPU_Task* task); // This is part of VPU_Environment now
// Forward declaration for VPU_Environment methods will be added below.
  ```
- Language Bindings: Core C-API with wrappers (Python, Rust, C++).

##### 2. JIT/WASM Interface:
- Function: Portable, secure, platform-independent format for kernels.
- Workflow: Developers compile functions to WASM. VPU_Task points to WASM binary. VPU acts as specialized WASM runtime.

##### 3. Deep OS Interception Hooks (The "Trapper"):
- Function: System-wide accelerator for existing, unmodified applications.
- Mechanism: Privileged daemon/kernel module using LD_PRELOAD or syscall interception.
- Target Libraries: BLAS/LAPACK, FFTW/cuFFT, libc (memcpy), Media Codecs.

#### Data Flow

Application initiates task -> Captured by High-Level API or OS Interceptor -> Task & data packaged into VPU_Task structure -> Placed in high-priority queue for VPU core logic.

### Pillar 2: The Flux Profiler & Analyzer (The "Cerebral Cortex")
*(Alias: Perception Layer)*

This is the VPU's primary perception layer. It takes the raw VPU_Task and enriches it with a deep understanding of its context, answering: 1. What is the intrinsic nature and complexity of this data? (Representational Flux Analysis) 2. What are the capabilities and costs of the environment? (Hardware Capability Analysis). Output is an "Enriched Execution Context".

#### Architecture

##### 1. Representational Flux Analyzer (τ_W Module):
- Function: Quantify intrinsic data cost (Arbitrary Contextual Weight - ACW).
- Sub-modules:
  - Digital WFC Profiler (Low-Level): For raw binary ops. Calculates Hamming Weight (HW). Output: `WFC_Profile {'{total_bit_count, active_bit_count (HW), sparsity_ratio}'}`.
  - Omnimorphic Data Profiler (High-Level): For generic data streams. Computes: Amplitude Flux A(W), Frequency Flux F(W), Entropy Flux E(W). Output: `OmniFlux_Profile {'{A_W, F_W, E_W}'}`.

##### 2. Hardware Capability Profiler & Database (The "Calibrator"):
- Function: Maintain dynamic understanding of operational costs on host.
- Operational Modes: Offline (benchmarks), Online (learning from Pillar 5).
- Database (Hardware Profile): Key-value store.
  - τ_operation Table: Cost of specific operation. Example: `{"{key: ('ADD', 'FP32_VECTOR', 'CPU_AVX2'), value: {flux_cost: 1.5, latency_ns_per_element: 0.3}}"}`
  - τ_transform Table: Cost of changing data representations. Example: `{"{key: ('TimeDomain', 'FrequencyDomain', 4096, 'CPU'), value: {flux_cost: 25000, latency_us: 15.0}}"}`

#### Data Flow

VPU_Task enters Cortex -> Representational Flux Analyzer profiles data (WFC_Profile or OmniFlux_Profile) -> Profile attached to task -> Cortex consults Hardware Capability Database for known costs (τ_operation, τ_transform) -> Bundled into "Enriched Execution Context" -> Passed to Pillar 3.

### Pillar 3: The Adaptive Orchestrator (The "Thalamus")
*(Alias: Cognitive Core)*

The Orchestrator is the cognitive core of the VPU. It decides how computation should be performed, not performing it itself. Input: "Enriched Execution Context" from Pillar 2. Purpose: Evaluate potential computational strategies and select the path predicted to consume minimum Holistic Flux.

#### Architecture

##### 1. Candidate Path Generator:
- Function: Create valid, potential strategies for the task.
- Logic: Rule-based expert system. Example for "Convolution": Path 1 (Direct Time Domain), Path 2 (FFT-based Frequency Domain), Path 3 (Wavelet Domain Sparsity-based).
- Output: Structured sequence of (Operation, Representation) tuples.

##### 2. Flux Cost Simulator:
- Function: Calculate predicted Holistic_Flux for each candidate path.
- Logic: For each path: Tally τ_transform costs from Hardware Profile. Calculate τ_operation = Base_Op_Cost_CU * f(ACW). Sum for Holistic Flux = Σ(τ_transform) + Predicted_τ_operation.

##### 3. Decision Engine:
- Function: Make final selection.
- Logic: Compares Holistic_Flux values, selects path with absolute minimum. Resolves trade-off between transformation cost and operational efficiency.

#### Core Structures (ExecutionPlan Example)

```json
{
  "optimal_path": "Frequency Domain",
  "predicted_holistic_flux": 8450.0,
  "steps": [
    { "operation_name": "TRANSFORM", "input_buffer_id": "input_data", "output_buffer_id": "fft_output", "kernel_details": "FFT_4096_GPU_0" },
    { "operation_name": "EXECUTE", "input_buffer_id": "fft_output", "output_buffer_id": "multiplied_output", "kernel_details": "ELEMENT_WISE_MULTIPLY_FP32_COMPLEX_GPU_0" },
    { "operation_name": "TRANSFORM", "input_buffer_id": "multiplied_output", "output_buffer_id": "final_output", "kernel_details": "IFFT_4096_GPU_0" }
  ]
}
```

#### Data Flow

Enriched_Execution_Context arrives -> Candidate Path Generator creates strategies -> Flux Cost Simulator annotates with Holistic_Flux score -> Decision Engine identifies winning path -> Creates definitive ExecutionPlan.

### Pillar 4: The Polymorphic JIT Runtime (The "Cerebellum")
*(Alias: Engine of Action)*

This is the VPU's engine of action and motor control. It takes the abstract ExecutionPlan from the Orchestrator and transforms it into highly optimized, high-performance execution on physical hardware. "Polymorphic" refers to its ability to change execution strategy. "JIT" (Just-In-Time) capability allows generating uniquely specialized code.

#### Architecture

##### 1. Execution Dispatcher:
- Reads ExecutionPlan, orchestrates operations, manages dependencies.

##### 2. Hardware Abstraction Layer (HAL) & Kernel Library:
- Repository of pre-compiled, "best-in-class" computational primitives.
- Implementations: CPU Kernels (SIMD intrinsics), GPU Kernels (CUDA, ROCm, Metal), WASM Runtime (e.g., Wasmtime).

##### 3. Data Flow & Memory Manager:
- Manages physical location and movement of data.
- Logic: Substrate-Awareness (CPU RAM, GPU VRAM), Efficient Transfer (pinned memory), Cost Accounting (feedback to Pillar 5).

##### 4. Polymorphic JIT (Just-In-Time) Compiler:
- Generates new, temporary machine code on the fly when pre-compiled kernels are insufficient.
- Technology: Built on LLVM. Generates LLVM IR, uses LLVM backend for optimized code.
- Flux-Adaptive Code Generation: Sparsity Exploitation (O(NNZ) from WFC_Profile), Precision Pruning (e.g., FP16), Kernel Fusion.

#### Data Flow

ExecutionPlan received by Dispatcher -> Step 1 (e.g., FFT_4096 on GPU): Data Flow Manager moves data to GPU, invokes FFT kernel from HAL's GPU library -> Step 2 (e.g., ELEMENT_WISE_MULTIPLY): If sparse, JIT Compiler might generate custom kernel. Otherwise, simple GPU kernel -> Final result moved back to CPU RAM.

### Pillar 5: The Flux-Quark Feedback Loop (The "Hippocampus")
*(Alias: Memory and Learning)*

This pillar is the VPU's mechanism for memory, learning, and self-correction, completing the cognitive cycle: Perceive -> Decide -> Act -> LEARN. It compares predicted cost against actual, measured cost. A "Flux Quark" (significant discrepancy) triggers updates to VPU's internal "beliefs" to improve model accuracy. Adapts to hardware quirks, OS updates, thermal throttling.

#### Architecture

##### 1. Post-Execution Instrumentation Monitor (The "Observer"):
- Function: Capture ground truth of completed computation.
- Implementation: Dispatcher (Pillar 4) wraps ExecutionPlan steps in timers and performance counters.
- Data Collected: Wall-clock latency, Substrate-specific metrics (CPU: instructions retired, cache misses; GPU: kernel time, memory R/W), Power Usage (via RAPL/NVML). Packaged into ActualPerformanceRecord.

##### 2. Deviation Analyzer (The "Auditor"):
- Function: Compare prediction against reality, detect "Quarks."
- Logic: Takes ExecutionPlan (predicted_flux) and ActualPerformanceRecord. Converts raw metrics to Observed_Flux. Calculates Deviation = (Observed_Flux - Predicted_Flux) / Predicted_Flux. If abs(Deviation) > FLUX_QUARK_THRESHOLD, flags learning opportunity.

##### 3. Belief Update Manager (The "Learning Core"):
- Function: Refine VPU's internal models (Hardware Capability Database in Pillar 2 & 3).
- Logic: Root Cause Analysis (identify if error was in τ_transform or τ_operation). Adaptive Learning Rule (e.g., New_Cost = Old_Cost * (1 - learning_rate) + Observed_Cost * learning_rate).
- Mechanism: If VPU too optimistic (Observed > Predicted), penalizes by increasing stored cost. If too pessimistic, rewards by decreasing cost.

#### Data Flow

A VPU_Task enters Synapse (1) -> Cortex (2) profiles, creates Enriched_Execution_Context -> Orchestrator (3) creates predictive ExecutionPlan -> Cerebellum (4) executes, generating ActualPerformanceRecord -> Feedback Loop (5) compares Plan and Record. If Quark detected, Learning Core updates beliefs in Cortex (2) -> Next similar task benefits from improved accuracy.

## VPU Technical Specification - Version 1.0

### Core Data Structures

#### VPU_Task
```c
typedef struct VPU_Task { // Aligned with api/vpu.h
  // Task Identifier
  uint64_t task_id;      // Unique identifier for the task
  std::string task_type; // e.g., "CONVOLUTION", "GEMM", "SAXPY", or a more generic "USER_DEFINED_KERNEL"

  // Kernel Definition
  enum class KernelType {
      FUNCTION_POINTER,
      WASM_BINARY
  };
  KernelType kernel_type;

  union Kernel {
      // Signature: (input_a, input_b, output, num_elements)
      void (*function_pointer)(const void* data_in_a, const void* data_in_b, void* data_out, size_t num_elements);
      const uint8_t* wasm_binary; // Pointer to WASM module binary data

      Kernel() : function_pointer(nullptr) {} // Default constructor for union
  } kernel;

  size_t kernel_size; // Optional: Size of the WASM binary, if kernel_type is WASM_BINARY

  // Data payload
  const void* data_in_a;
  const void* data_in_b;
  void* data_out;
  size_t num_elements; // Relevant for array/vector operations

  // Default constructor to initialize members
  VPU_Task() : task_id(0), kernel_type(KernelType::FUNCTION_POINTER), kernel_size(0),
               data_in_a(nullptr), data_in_b(nullptr), data_out(nullptr), num_elements(0) {}
} VPU_Task;
```

#### EnrichedExecutionContext
```c
typedef struct EnrichedExecutionContext {
  // VPU_Task original_task; // This is not directly in EnrichedExecutionContext as per vpu_data_structures.h
                          // Instead, profile and task_type are primary members.
  std::shared_ptr<const DataProfile> profile; // Profile of the input data
  std::string task_type;                      // Type of the task (e.g., "CONVOLUTION")

  // The hardware_profile is not part of EnrichedExecutionContext directly,
  // but is used by Pillar3_Orchestrator when processing it.
} EnrichedExecutionContext;
```

#### ExecutionPlan (JSON Example)
```json
{
  "chosen_path_name": "Path_Name_Example", // Name of the chosen execution path
  "predicted_holistic_flux": 3104.5,
  "steps": [ // Sequence of operations to perform
    { "operation_name": "MOVE_TO_GPU", "input_buffer_id": "data_in", "output_buffer_id": "gpu_mem_1" },
    { "operation_name": "FFT_FORWARD", "input_buffer_id": "gpu_mem_1", "output_buffer_id": "gpu_mem_2" },
    // ... more steps ...
    { "operation_name": "MOVE_FROM_GPU", "input_buffer_id": "gpu_mem_final", "output_buffer_id": "data_out_buffer" }
  ]
}
```

### VPU_Environment
The `VPU_Environment` class encapsulates the VPU runtime, providing an interface to execute tasks and manage the VPU's lifecycle.

```c++
namespace VPU {
class VPU_Environment {
public:
    VPU_Environment(); // Constructor: Initializes the VPUCore and its pillars.
    ~VPU_Environment(); // Destructor: Handles cleanup of VPU resources.

    // Executes a given VPU_Task.
    // The task is processed through the VPU's cognitive cycle (Perceive, Decide, Act, Learn).
    void execute(VPU_Task& task);

    // Dumps the VPU's current internal beliefs (e.g., learned costs from the HardwareProfile)
    // to the console for debugging or inspection.
    void print_beliefs();

    // Provides access to the internal VPUCore instance.
    // Intended primarily for testing and advanced scenarios.
    VPUCore* get_core_for_testing();
private:
    std::unique_ptr<VPUCore> core; // Pointer to the main VPU implementation.
};
} // namespace VPU
```

### ActualPerformanceRecord
This structure records the actual measured performance metrics after a task has been executed. It's used by Pillar 5 (Feedback Loop) to compare against predicted performance.
```c
struct ActualPerformanceRecord {
    double observed_holistic_flux = 0.0; // The actual holistic flux calculated from performance counters.
    // Other metrics like wall-clock time, power usage can be added here.
};
```

### LearningContext
This structure provides necessary information to the learning algorithms within Pillar 5. It helps pinpoint the source of any discrepancy between predicted and actual performance.
```c
struct LearningContext {
    std::string path_name;           // The name of the execution path taken.
    std::string transform_key;       // Key identifying the transformation step, if applicable.
    std::string operation_key;       // Key for sensitivity (lambda) learning.
    std::string main_operation_name; // Key for base operational cost learning.
};
```

## Pillar 1: The Universal Flux API & Interceptor (The "Synapse")
Pillar 1, also known as the "Synapse," serves as the VPU's outermost layer and primary interface for external applications. It is responsible for intercepting computational tasks, packaging them into a standardized `VPU_Task` format, and forwarding them to the VPU's core processing pipeline. This pillar aims to provide a seamless, hardware-agnostic API that allows new and existing software to leverage the VPU's optimization capabilities without significant modification. It supports various interception methods, including a high-level API library (libvpu), a JIT/WASM interface for portable kernels, and potentially deep OS interception hooks for transparently accelerating unmodified applications.

Key responsibilities include:
- **Task Reception:** Accepting computational tasks from applications via its defined APIs.
- **Task Validation:** Performing initial checks on the submitted tasks to ensure they are well-formed and contain necessary information.
- **Standardization:** Converting tasks into the common `VPU_Task` structure used throughout the VPU.
- **Forwarding:** Passing the standardized tasks to Pillar 2 (Cortex) for profiling and further analysis.

The `Pillar1_Synapse` class encapsulates this functionality:
```cpp
namespace VPU {
class Pillar1_Synapse {
public:
    // Constructor: Initializes the Synapse.
    // May include setup for different interception mechanisms or connections to Pillar 2.
    Pillar1_Synapse();

    // Public API method to submit a task for VPU processing.
    // It takes a 'VPU_Task' object (defined in api/vpu.h),
    // performs validation, and then queues or sends it to the next stage (Pillar 2).
    // Returns true if the task is accepted, false otherwise.
    bool submit_task(const VPU_Task& task);
private:
    // Internal helper method to validate the incoming VPU_Task.
    // Checks for required fields, valid data pointers, supported kernel types, etc.
    bool validate_task(const VPU_Task& task) const;
};
} // namespace VPU
```

## Pillar 2 Implementation: RepresentationalFluxAnalyzer

```cpp
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
* The definition below aligns with `src/vpu_data_structures.h`.
*/
struct DataProfile {
    // WFC Profile (Digital Binary Domain)
    uint64_t hamming_weight = 0;
    double sparsity_ratio = 1.0; // 1.0 = all zeros, 0.0 = all ones

    // Omnimorphic Profile (Numerical Sequence Domain)
    double amplitude_flux = 0.0;
    double frequency_flux = 0.0;
    double entropy_flux = 0.0;

    // IoT Sensor Data
    double power_draw_watts = 0.0;
    double temperature_celsius = 0.0;
    double network_latency_ms = 0.0;
    double network_bandwidth_mbps = 0.0;
    double io_throughput_mbps = 0.0;
    double data_quality_score = 1.0; // Default to perfect quality
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
    * @return A DataProfile object populated with omnimorphic flux metrics (amplitude_flux, frequency_flux, entropy_flux).
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
```

### Pillar 2 Example Usage
```cpp
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
    std::cout << "Omnimorphic Profile (from src/vpu_data_structures.h):" << std::endl;
    std::cout << " - Amplitude Flux: " << profile.amplitude_flux << std::endl;
    std::cout << " - Frequency Flux: " << profile.frequency_flux << std::endl;
    std::cout << " - Entropy Flux: " << profile.entropy_flux << std::endl;
    // Displaying IoT fields as well, if populated
    if (profile.power_draw_watts > 0)
        std::cout << " - Power Draw (Watts): " << profile.power_draw_watts << std::endl;
    if (profile.temperature_celsius > 0)
        std::cout << " - Temperature (Celsius): " << profile.temperature_celsius << std::endl;
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
    // Note: The 'profileOmni' method in RepresentationalFluxAnalyzer uses internal fields
    // like 'amplitude_flux_A_W'. The 'DataProfile' struct it returns will have these values
    // mapped to 'amplitude_flux', 'frequency_flux', 'entropy_flux' by the method itself,
    // or the print_profile function should be aware of the internal names if accessing raw results.
    // For this example, we assume 'profileOmni' populates the standardized fields correctly.
    std::cout << "\\n===== SCENARIO 2: Omnimorphic Sequence Profiling =====" << std::endl;
    const size_t n_samples = 1024;
    std::vector<double> smooth_signal(n_samples);
    std::vector<double> noisy_signal(n_samples);

    srand(time(0)); // Seed random number generator

    for(size_t i = 0; i < n_samples; ++i) {
        smooth_signal[i] = std::sin(2 * M_PI * 5 * static_cast<double>(i) / n_samples); // Smooth 5 Hz sine wave
        noisy_signal[i] = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0; // White noise
    }

    // Assuming analyzer.profileOmni correctly populates the standardized DataProfile fields:
    auto smooth_omni_profile = analyzer.profileOmni(smooth_signal.data(), n_samples);
    // Example: Manually populate IoT fields for demonstration, as profileOmni doesn't do this.
    // smooth_omni_profile.power_draw_watts = 15.5;
    // smooth_omni_profile.temperature_celsius = 25.0;
    std::cout << "Profile for 'Smooth Signal' (Sine Wave):";
    print_profile(smooth_omni_profile);

    auto noisy_omni_profile = analyzer.profileOmni(noisy_signal.data(), n_samples);
    std::cout << "Profile for 'Noisy Signal' (White Noise):";
    print_profile(noisy_omni_profile);

    return 0;
}
*/
```

## Pillar 6: Task Graph Orchestrator
Pillar 6, the Task Graph Orchestrator, introduces a higher level of dynamic optimization to the VPU by identifying and exploiting patterns in task execution sequences. Its primary function is to observe the flow of operations over time and detect frequently co-occurring sequences of tasks (or steps within tasks) that can be "fused" into a single, more optimized kernel. This process is akin to automatic code optimization or macro-operation formation, tailored to the specific workload being processed by the VPU.

Key functionalities include:
- **Execution History Tracking:** Recording metadata from `ExecutionPlan`s (from Pillar 3) to build a history of operations.
- **Pattern Analysis:** Periodically analyzing this history to identify sequences of operations that appear together more often than a defined threshold.
- **Kernel Fusion:** For identified frequent sequences, Pillar 6 can initiate the creation of a new "fused kernel." This involves:
    - Defining the interface and implementation of the new kernel (potentially by combining existing kernel code or generating new code via JIT compilation from Pillar 4).
    - Registering the new fused kernel with the `HAL::KernelLibrary`.
    - Updating the `HardwareProfile` (in Pillar 2/3) with the performance characteristics and cost model of this new fused kernel.
- **Adaptive Optimization:** Once a fused kernel is available, Pillar 3 (Orchestrator) can consider it as a candidate path for future tasks, potentially leading to more efficient execution by reducing overhead (e.g., data movement, kernel launch latency).

The `TaskGraphOrchestrator` class implements this logic:
```cpp
namespace VPU {
class TaskGraphOrchestrator {
public:
    // Constructor: Initializes the orchestrator.
    // Takes a shared pointer to the KernelLibrary (to register new fused kernels)
    // and the HardwareProfile (to update with costs of new kernels).
    // 'fusion_candidate_threshold' defines how many times a sequence must appear to be considered for fusion.
    TaskGraphOrchestrator(std::shared_ptr<HAL::KernelLibrary> kernel_lib,
                          std::shared_ptr<HardwareProfile> hw_profile,
                          int fusion_candidate_threshold = 10);

    // Called by the VPU core after a task's ExecutionPlan has been processed by Pillar 4.
    // This method stores relevant information from the plan for future analysis.
    void record_executed_plan(const ExecutionPlan& plan);

    // This method is triggered periodically (e.g., after a certain number of tasks
    // have been executed, or during idle periods). It analyzes 'plan_history_'
    // to find frequent operation sequences and attempts to create fused kernels.
    void analyze_and_fuse_patterns();

    // Test helpers to allow fine-tuning of fusion parameters and resetting state during tests.
    void set_fusion_candidate_threshold_for_testing(int threshold); // Adjusts how often a pattern must be seen.
    void set_analysis_interval_for_testing(int interval); // Adjusts how many tasks trigger an analysis.
    void reset_task_execution_counter_for_testing(); // Resets the counter for periodic analysis.

private:
    // Internal method to scan 'plan_history_' and count occurrences of (op1_name, op2_name) sequences.
    // Returns a map where keys are pairs of operation names and values are their frequencies.
    std::map<std::pair<std::string, std::string>, int> find_frequent_sequences();

    // Internal method called when a frequent sequence is identified.
    // This method is responsible for the logic of creating the new fused kernel.
    // It would interact with the KernelLibrary to add the new kernel and
    // the HardwareProfile to provide its estimated cost.
    void create_fused_kernel(const std::string& op1_name, const std::string& op2_name);

    // Internal State:
    std::vector<ExecutionPlan> plan_history_; // Stores past execution plans.
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_; // Interface to the VPU's available kernels.
    std::shared_ptr<HardwareProfile> hw_profile_;   // Interface to the VPU's hardware cost database.
    int fusion_candidate_threshold_; // Minimum count for a sequence to be a fusion candidate.
    int task_execution_counter_;     // Counts tasks executed since last analysis.
    int analysis_interval_;          // Number of tasks between analyses.
};
} // namespace VPU
```
