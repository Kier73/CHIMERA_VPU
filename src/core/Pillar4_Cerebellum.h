#pragma once

#include "vpu_data_structures.h"
#include "hal/hal.h"
#include <vector>

namespace VPU {

// CONCEPTUAL JIT ENGINE
// In a real system, this would be a complex class using LLVM.
// Here, it's a conceptual placeholder demonstrating the "build a new tool" trade-off.
class FluxJITEngine {
public:
    // This function decides if generating a custom kernel is worthwhile.
    HAL::GenericKernel compile_saxpy_for_data(const std::vector<float>& data);
};

// The Cerebellum is the engine of action. It takes a plan and makes it reality.
class Cerebellum {
public:
    explicit Cerebellum(std::shared_ptr<HAL::KernelLibrary> kernel_lib);
    ActualPerformanceRecord execute(const ExecutionPlan& plan, VPU_Task& task);

private:
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;
    FluxJITEngine jit_engine_;
};

} // namespace VPU
