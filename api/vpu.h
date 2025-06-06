#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint> // For uint64_t, uint8_t

namespace VPU {

// Forward declaration of the main implementation class to hide details.
class VPUCore;

// Represents a computational task and its data payload.
// This is the primary structure used by a developer to submit work.
struct VPU_Task {
    uint64_t task_id;      // Unique identifier for the task
    std::string task_type; // e.g., "CONVOLUTION", "GEMM", "SAXPY", or a more generic "USER_DEFINED_KERNEL"

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

        // It's good practice to initialize a union member, especially if it contains pointers.
        Kernel() : function_pointer(nullptr) {}
    } kernel;

    size_t kernel_size; // Optional: Size of the WASM binary, if kernel_type is WASM_BINARY

    // Data payload - can be used by function_pointer kernels directly,
    // or as general data buffers for WASM kernels (WASM would read from/write to these via VPU runtime).
    const void* data_in_a;
    const void* data_in_b;
    void* data_out;
    size_t num_elements; // Relevant for array/vector operations, context for data_in/out pointers
    size_t data_in_a_size_bytes; // Size of data_in_a in bytes
    size_t data_in_b_size_bytes; // Size of data_in_b in bytes


    // Default constructor to initialize members
    VPU_Task() : task_id(0), kernel_type(KernelType::FUNCTION_POINTER), kernel_size(0),
                 data_in_a(nullptr), data_in_b(nullptr), data_out(nullptr), num_elements(0),
                 data_in_a_size_bytes(0), data_in_b_size_bytes(0) {}
};

// Represents the VPU runtime environment.
// Hides the complexity of the VPUCore implementation.
class VPU_Environment {
public:
    VPU_Environment();
    ~VPU_Environment();

    // The primary function to execute a task.
    // Note: Task is now passed by value or const reference if it's complex,
    // to avoid issues if the VPU processes it asynchronously.
    // For now, mutable reference is fine as processing is synchronous.
    void execute(VPU_Task& task);

    // Dumps the VPU's current internal beliefs for inspection.
    void print_beliefs();

    // Method to get access to VPUCore for testing purposes
    VPUCore* get_core_for_testing();

    // Method to get the last performance record for testing
    const ActualPerformanceRecord& get_last_performance_record() const;

private:
    std::unique_ptr<VPUCore> core;
};

} // namespace VPU
