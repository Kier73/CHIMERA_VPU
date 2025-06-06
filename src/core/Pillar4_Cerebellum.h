#pragma once

#include "vpu_data_structures.h"
#include "hal/hal.h"
#include "vpu.h" // Added: For VPU_Task definition
#include <vector>
#include <functional> // For std::function (HAL::GenericKernel)

namespace VPU {

// CONCEPTUAL JIT ENGINE
// In a real system, this would be a complex class using LLVM.
// Here, it's a conceptual placeholder demonstrating the "build a new tool" trade-off.
class FluxJITEngine {
public:
    FluxJITEngine(); // Added constructor
    // This function decides if generating a custom kernel is worthwhile.
    // JIT kernels are fully specialized and capture their data, so they are nullary.
    std::function<HAL::KernelFluxReport()> compile_saxpy_for_data(VPU_Task& task);

    // Method to enable/disable LLM usage for JIT
    void set_llm_jit_generation(bool enable);

private:
    // (Conceptual) Method to generate kernel with LLM
    // Also returns a nullary, fully specialized JIT kernel.
    std::function<HAL::KernelFluxReport()> generate_kernel_with_llm(const VPU_Task& task);

    // Member variable to control LLM usage for JIT
    bool use_llm_for_jit_;
};

// The Cerebellum is the engine of action. It takes a plan and makes it reality.
class Cerebellum {
public:
    explicit Cerebellum(std::shared_ptr<HAL::KernelLibrary> kernel_lib);
    ActualPerformanceRecord execute(const ExecutionPlan& plan, VPU_Task& task);

    // Test helper to access JIT engine
    FluxJITEngine* get_jit_engine_for_testing() { return &jit_engine_; }

private:
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_; // Stores std::function<KernelFluxReport(VPU_Task& task)>
    FluxJITEngine jit_engine_;
    std::function<HAL::KernelFluxReport()> last_jit_compiled_kernel_; // JIT kernel is nullary
};

} // namespace VPU
