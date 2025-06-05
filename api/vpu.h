#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace VPU {

// Forward declaration of the main implementation class to hide details.
class VPUCore;

// Represents a computational task and its data payload.
// This is the primary structure used by a developer to submit work.
struct VPU_Task {
    std::string task_type; // e.g., "CONVOLUTION", "GEMM", "SAXPY"

    // For simplicity, we use generic data pointers.
    // A real system might use a more complex data handle system.
    const void* data_in_a;
    const void* data_in_b;
    void* data_out;

    size_t num_elements;
    // Other metadata...
};

// Represents the VPU runtime environment.
// Hides the complexity of the VPUCore implementation.
class VPU_Environment {
public:
    VPU_Environment();
    ~VPU_Environment();

    // The primary function to execute a task.
    void execute(VPU_Task& task);

    // Dumps the VPU's current internal beliefs for inspection.
    void print_beliefs();

private:
    std::unique_ptr<VPUCore> core;
};

} // namespace VPU
