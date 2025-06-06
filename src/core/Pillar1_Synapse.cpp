#include "Pillar1_Synapse.h"
#include <iostream> // For logging/placeholder messages

// For now, Pillar1 doesn't directly talk to Pillar2 in this basic implementation.
// This would be where you include Pillar2_Cortex.h if Pillar1 handed off to it.
// #include "Pillar2_Cortex.h"

namespace VPU {

// Constructor
Pillar1_Synapse::Pillar1_Synapse() {
    // Initialization, if any.
    // If Pillar1 needs to pass tasks to Pillar2, Pillar2 instance could be passed here.
    // e.g., this->next_pillar_cortex = cortex;
    std::cout << "[Pillar1_Synapse] Initialized." << std::endl;
}

// Accepts a task, performs initial validation/processing,
// and forwards it for deeper analysis and execution.
bool Pillar1_Synapse::submit_task(const VPU_Task& task) {
    std::cout << "[Pillar1_Synapse] Received task ID: " << task.task_id
              << " of type: " << task.task_type << std::endl;

    if (!validate_task(task)) {
        std::cerr << "[Pillar1_Synapse] Task ID: " << task.task_id << " failed validation." << std::endl;
        return false;
    }

    std::cout << "[Pillar1_Synapse] Task ID: " << task.task_id << " validated successfully." << std::endl;

    // Process based on kernel type
    switch (task.kernel_type) {
        case VPU_Task::KernelType::FUNCTION_POINTER:
            if (task.kernel.function_pointer) {
                std::cout << "[Pillar1_Synapse] Task " << task.task_id
                          << ": Ready for FUNCTION_POINTER dispatch (actual call deferred to Pillar4)."
                          << std::endl;
                // In a full system, this task (or a representation of it) would be passed to Pillar2 (Cortex)
                // for profiling and then to Pillar3 (Orchestrator) for planning.
                // For now, Pillar1's job is just to accept and validate.
            } else {
                std::cerr << "[Pillar1_Synapse] Task " << task.task_id
                          << ": FUNCTION_POINTER type selected, but function_pointer is null." << std::endl;
                return false;
            }
            break;
        case VPU_Task::KernelType::WASM_BINARY:
            std::cout << "[Pillar1_Synapse] Task " << task.task_id
                      << ": WASM_BINARY kernel type - Not yet implemented." << std::endl;
            // Placeholder for WASM loading and preparation logic.
            // This would involve interacting with a WASM runtime.
            // For now, we'll just acknowledge it.
            if (!task.kernel.wasm_binary || task.kernel_size == 0) {
                 std::cerr << "[Pillar1_Synapse] Task " << task.task_id
                           << ": WASM_BINARY type selected, but wasm_binary pointer is null or kernel_size is 0." << std::endl;
                 return false;
            }
            break;
        default:
            std::cerr << "[Pillar1_Synapse] Task " << task.task_id
                      << ": Unknown kernel type." << std::endl;
            return false;
    }

    // Placeholder: If submit_task makes it here, it means the task is valid from Pillar1's perspective
    // and is notionally passed on.
    std::cout << "[Pillar1_Synapse] Task " << task.task_id << " accepted for further processing." << std::endl;
    return true;
}

// Helper for basic validation
bool Pillar1_Synapse::validate_task(const VPU_Task& task) const {
    if (task.task_type.empty()) {
        std::cerr << "[Pillar1_Synapse Validation] Task type is empty." << std::endl;
        return false;
    }

    switch (task.kernel_type) {
        case VPU_Task::KernelType::FUNCTION_POINTER:
            if (!task.kernel.function_pointer) {
                std::cerr << "[Pillar1_Synapse Validation] Function pointer is null for FUNCTION_POINTER type." << std::endl;
                return false;
            }
            // Basic check for data pointers if num_elements > 0.
            // More sophisticated checks might be needed depending on task_type.
            if (task.num_elements > 0) {
                if (!task.data_in_a && !task.data_in_b) { // At least one input if elements exist
                    // This rule might be too strict, depends on kernel semantics.
                    // For a generic kernel, it's hard to say. Some kernels might only have one input or even none (generators).
                    // std::cerr << "[Pillar1_Synapse Validation] Data input pointers are null when num_elements > 0." << std::endl;
                    // return false;
                }
                if (!task.data_out) {
                    std::cerr << "[Pillar1_Synapse Validation] Data output pointer is null when num_elements > 0." << std::endl;
                    return false;
                }
            }
            break;
        case VPU_Task::KernelType::WASM_BINARY:
            if (!task.kernel.wasm_binary) {
                std::cerr << "[Pillar1_Synapse Validation] WASM binary pointer is null for WASM_BINARY type." << std::endl;
                return false;
            }
            if (task.kernel_size == 0) {
                std::cerr << "[Pillar1_Synapse Validation] Kernel size is 0 for WASM_BINARY type." << std::endl;
                return false;
            }
            break;
        default:
            std::cerr << "[Pillar1_Synapse Validation] Unknown kernel type specified." << std::endl;
            return false; // Should not happen if enum class is used correctly
    }
    return true;
}

} // namespace VPU
