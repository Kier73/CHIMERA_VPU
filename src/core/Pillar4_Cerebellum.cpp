#include "core/Pillar4_Cerebellum.h"
#include "hal/hal_utils.h" // For calculate_data_hamming_weight (will be used later)
#include <chrono>
#include <stdexcept> // Required for std::runtime_error
#include <iostream>  // Required for std::cout
#include <vector>    // Required for std::vector (will be used later)

namespace VPU {

Cerebellum::Cerebellum(std::shared_ptr<HAL::KernelLibrary> kernel_lib) : kernel_lib_(kernel_lib) {
     if (!kernel_lib_) {
        throw std::runtime_error("Cerebellum's KernelLibrary cannot be null.");
    }
}

// This function receives the final plan and executes it.
ActualPerformanceRecord Cerebellum::execute(const ExecutionPlan& plan, VPU_Task& task) {
    std::cout << "[Pillar 4] Cerebellum: Beginning execution of plan '" << plan.chosen_path_name << "'." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    last_jit_compiled_kernel_ = nullptr; // This type is now std::function<KernelFluxReport()>

    std::map<std::string, void*> memory_buffers;
    memory_buffers["input"] = const_cast<void*>(task.data_in_a);
    memory_buffers["output"] = task.data_out;

    std::vector<double> temp_buffer_1_double;
    std::vector<double> temp_buffer_2_double;
    // std::vector<float> temp_buffer_1_float; // Not used in current example path
    // std::vector<float> temp_buffer_2_float; // Not used in current example path

    if (task.task_type == "CONVOLUTION" && task.num_elements > 0) { // Example, not fully plumbed
        temp_buffer_1_double.resize(task.num_elements);
        temp_buffer_2_double.resize(task.num_elements);
        memory_buffers["temp_freq"] = temp_buffer_1_double.data();
        memory_buffers["temp_result"] = temp_buffer_2_double.data();
    }

    uint64_t total_cycle_cost = 0;
    uint64_t total_hw_in_cost = 0;
    uint64_t total_hw_out_cost = 0;
    HAL::KernelFluxReport report_from_kernel;

    for (const auto& step : plan.steps) {
        std::cout << "  -> Dispatching Step: " << step.operation_name << std::endl;
        report_from_kernel = {0,0,0}; // Reset report for steps that don't generate one (e.g. JIT_COMPILE)

        if (step.operation_name == "JIT_COMPILE_SAXPY") {
            std::cout << "  -> [Cerebellum] Requesting JIT compilation for SAXPY..." << std::endl;
            last_jit_compiled_kernel_ = jit_engine_.compile_saxpy_for_data(task);
            // JIT compilation step itself doesn't return a flux report in this context.
            // The cost of JIT compilation could be tracked separately if needed.
        } else if (step.operation_name == "EXECUTE_JIT_SAXPY") {
            if (last_jit_compiled_kernel_) {
                std::cout << "  -> [Cerebellum] Executing JIT-compiled SAXPY kernel..." << std::endl;
                report_from_kernel = last_jit_compiled_kernel_(); // JIT kernel now returns a report
            } else {
                std::cerr << "  -> [Cerebellum ERROR] EXECUTE_JIT_SAXPY called but no JIT kernel was compiled!" << std::endl;
                throw std::runtime_error("EXECUTE_JIT_SAXPY called without a compiled JIT kernel.");
            }
        } else if (kernel_lib_->count(step.operation_name)) {
            auto kernel_func = kernel_lib_->at(step.operation_name); // Type is std::function<KernelFluxReport(VPU_Task& task)>
            report_from_kernel = kernel_func(task); // Standard kernels now pass the task
        } else {
            throw std::runtime_error("Kernel not found in library: " + step.operation_name);
        }

        total_cycle_cost += report_from_kernel.cycle_cost;
        total_hw_in_cost += report_from_kernel.hw_in_cost;
        total_hw_out_cost += report_from_kernel.hw_out_cost;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> latency_ns_duration = end_time - start_time;

    ActualPerformanceRecord result_record;
    result_record.observed_latency_ns = latency_ns_duration.count();
    result_record.observed_cycle_cost = total_cycle_cost;
    result_record.observed_hw_in_cost = total_hw_in_cost;
    result_record.observed_hw_out_cost = total_hw_out_cost;
    result_record.observed_holistic_flux = static_cast<double>(total_cycle_cost + total_hw_in_cost + total_hw_out_cost);

    std::cout << "  ==> Execution Complete. Observed Latency (ns): " << result_record.observed_latency_ns << std::endl;
    std::cout << "      Cycle Cost: " << result_record.observed_cycle_cost
              << ", HW IN Cost: " << result_record.observed_hw_in_cost
              << ", HW OUT Cost: " << result_record.observed_hw_out_cost << std::endl;
    std::cout << "      Holistic Flux: " << result_record.observed_holistic_flux << std::endl;

    return result_record;
}

// FluxJITEngine Implementation
FluxJITEngine::FluxJITEngine() : use_llm_for_jit_(false) {}

void FluxJITEngine::set_llm_jit_generation(bool enable) {
    use_llm_for_jit_ = enable;
    std::cout << "    -> [JIT Engine] LLM JIT generation " << (enable ? "enabled." : "disabled.") << std::endl;
}

// Return type is now std::function<HAL::KernelFluxReport()>
std::function<HAL::KernelFluxReport()> FluxJITEngine::generate_kernel_with_llm(const VPU_Task& task) {
    std::cout << "    -> [JIT Engine] LLM JIT kernel generation called for task: " << task.task_type << std::endl;
    // In a real scenario, this would involve:
    // 1. Formatting the task details (data profile, operation type, constraints) into a prompt.
    // 2. Sending the prompt to a code-generation LLM.
    // 3. Parsing the LLM's response (e.g., C++/CUDA/Metal code as a string).
    // 4. Compiling this code string dynamically (e.g., using NVRTC for CUDA, or system compiler for C++).
    // 5. Loading the compiled function and returning a std::function or similar callable.
    // For now, returning nullptr to signify conceptual implementation.
    return nullptr;
}

// Conceptual JIT engine logic
// Return type is now std::function<HAL::KernelFluxReport()>
std::function<HAL::KernelFluxReport()> FluxJITEngine::compile_saxpy_for_data(VPU_Task& task) {
    std::cout << "    -> [JIT Engine] SAXPY compilation request for task_type: " << task.task_type << std::endl;

    if (use_llm_for_jit_) {
        std::cout << "    -> [JIT Engine] Attempting LLM-based JIT generation..." << std::endl;
        // llm_kernel is now std::function<HAL::KernelFluxReport()>
        std::function<HAL::KernelFluxReport()> llm_kernel = generate_kernel_with_llm(task);
        if (llm_kernel) { // Check if a valid function was returned
            std::cout << "    -> [JIT Engine] LLM JIT generation successful (conceptually)." << std::endl;
            return llm_kernel;
        } else {
            std::cout << "    -> [JIT Engine] LLM JIT generation failed or not applicable/stubbed, falling back to traditional JIT." << std::endl;
        }
    }

    const float* x_data = static_cast<const float*>(task.data_in_a);
    float fixed_a = 1.0f; // Placeholder for SAXPY 'a' parameter from task if available

    std::vector<float> x_vec_analysis; // For sparsity check
    if (x_data && task.num_elements > 0) {
       x_vec_analysis.assign(x_data, x_data + task.num_elements);
    }

    size_t zero_count = 0;
    if (!x_vec_analysis.empty()) {
        for (float val : x_vec_analysis) {
            if (val == 0.0f) zero_count++;
        }
    }
    // Ensure num_elements is not zero to avoid division by zero if x_vec_analysis is empty due to num_elements being 0.
    double sparsity_ratio = x_vec_analysis.empty() ? 1.0 : static_cast<double>(zero_count) / x_vec_analysis.size(); // Default to fully sparse if empty
    std::cout << "    -> [JIT Engine] Data sparsity for input 'x': " << sparsity_ratio << std::endl;

    void* p_data_in_a = const_cast<void*>(task.data_in_a);
    void* p_data_out = task.data_out;
    size_t num_elements_captured = task.num_elements;

    // This lambda captures necessary variables and performs the SAXPY operation,
    // then calculates and returns the KernelFluxReport.
    auto saxpy_execution_lambda =
        [fixed_a, p_data_in_a, p_data_out, num_elements_captured](bool use_sparse_specialization) -> HAL::KernelFluxReport {

        if (!p_data_in_a || !p_data_out || num_elements_captured == 0) {
            std::cerr << "JIT KERNEL (SAXPY): Invalid data pointers or zero elements." << std::endl;
            return {0, 0, 0}; // Return zero flux report on error
        }

        const float* x_ptr = static_cast<const float*>(p_data_in_a);
        float* y_ptr = static_cast<float*>(p_data_out);

        // Create std::vector copies to pass to existing HAL functions.
        // This is for compatibility with HAL functions expecting vectors.
        // A more optimized JIT might work directly with pointers or have specialized data structures.
        std::vector<float> x_temp_vec(x_ptr, x_ptr + num_elements_captured);
        std::vector<float> y_temp_vec(y_ptr, y_ptr + num_elements_captured); // y is input/output for SAXPY

        HAL::KernelFluxReport report;

        // Calculate hw_in_cost: Hamming weight of input vector x and initial state of y
        report.hw_in_cost = HAL::calculate_data_hamming_weight(x_temp_vec.data(), x_temp_vec.size() * sizeof(float));
        report.hw_in_cost += HAL::calculate_data_hamming_weight(y_temp_vec.data(), y_temp_vec.size() * sizeof(float)); // y's initial state

        // Execute the appropriate SAXPY kernel
        if (use_sparse_specialization) {
            HAL::cpu_saxpy_sparse_specialized(fixed_a, x_temp_vec, y_temp_vec);
        } else {
            HAL::cpu_saxpy_dense_specialized(fixed_a, x_temp_vec, y_temp_vec);
        }

        // Copy the result from the temporary y_temp_vec back to the original memory location p_data_out
        for(size_t i = 0; i < num_elements_captured; ++i) {
            y_ptr[i] = y_temp_vec[i];
        }

        // Calculate hw_out_cost: Hamming weight of the output vector y (after computation)
        report.hw_out_cost = HAL::calculate_data_hamming_weight(y_temp_vec.data(), y_temp_vec.size() * sizeof(float));

        // Estimate cycle_cost: For SAXPY, operations are num_elements * (1 multiply + 1 add)
        report.cycle_cost = num_elements_captured * 2;

        return report;
    };

    if (sparsity_ratio > 0.5) {
        std::cout << "    -> [JIT Engine] Data is sparse. Providing 'SPARSE SAXPY' wrapper." << std::endl;
        // Return a lambda that calls saxpy_execution_lambda with use_sparse_specialization = true
        return [saxpy_execution_lambda]() -> HAL::KernelFluxReport {
            return saxpy_execution_lambda(true);
        };
    } else {
        std::cout << "    -> [JIT Engine] Data is dense. Providing 'DENSE SAXPY' wrapper." << std::endl;
        // Return a lambda that calls saxpy_execution_lambda with use_sparse_specialization = false
        return [saxpy_execution_lambda]() -> HAL::KernelFluxReport {
            return saxpy_execution_lambda(false);
        };
    }
}


} // namespace VPU
