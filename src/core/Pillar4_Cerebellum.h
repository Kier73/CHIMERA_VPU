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
    HAL::GenericKernel compile_saxpy_for_data(VPU_Task& task); // Modified signature

    // Method to enable/disable LLM usage for JIT
    void set_llm_jit_generation(bool enable);

private:
    // (Conceptual) Method to generate kernel with LLM
    HAL::GenericKernel generate_kernel_with_llm(const VPU_Task& task);

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
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;
    FluxJITEngine jit_engine_;
    HAL::GenericKernel last_jit_compiled_kernel_; // To store JIT kernel
};

} // namespace VPU
