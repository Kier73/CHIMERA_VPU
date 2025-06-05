#include "core/Pillar4_Cerebellum.h"
#include <chrono>
#include <stdexcept> // Required for std::runtime_error
#include <iostream>  // Required for std::cout
#include <any>       // For std::any_cast, std::bad_any_cast (though already in api/vpu.h)
#include "vpu_data_structures.h" // For VPU::SAXPYParams

namespace VPU {

Cerebellum::Cerebellum(std::shared_ptr<HAL::KernelLibrary> kernel_lib) : kernel_lib_(kernel_lib) {
     if (!kernel_lib_) {
        throw std::runtime_error("Cerebellum's KernelLibrary cannot be null.");
    }
}

// This function receives the final plan and executes it.
ActualPerformanceRecord Cerebellum::execute(const ExecutionPlan& plan, VPU_Task& task) {
    std::cout << "[Pillar 4] Cerebellum: Beginning execution of plan '" << plan.chosen_path_name << "'." << std::endl;

    // Use high-precision timer to capture ground truth
    auto start_time = std::chrono::high_resolution_clock::now();

    // Ensure last_jit_compiled_kernel_ is reset at the beginning of execution.
    last_jit_compiled_kernel_ = nullptr;

    // The Memory Manager holds intermediate results between steps.
    std::map<std::string, void*> memory_buffers;
    memory_buffers["input"] = const_cast<void*>(task.data_in_a);
    memory_buffers["output"] = task.data_out;

    // Allocate temporary buffers if needed
    // This is a simplified way to handle intermediate data for multi-step plans.
    // A real system would have a dedicated memory manager.
    std::vector<double> temp_buffer_1_double;
    std::vector<double> temp_buffer_2_double;
    std::vector<float> temp_buffer_1_float;
    std::vector<float> temp_buffer_2_float;

    // Basic type handling for temp buffers based on task type (conceptual)
    if (task.task_type == "CONVOLUTION" && task.num_elements > 0) { // FFTs in this example use double
        temp_buffer_1_double.resize(task.num_elements); // Adjusted for FFT output size if necessary
        temp_buffer_2_double.resize(task.num_elements);
        memory_buffers["temp_freq"] = temp_buffer_1_double.data();
        memory_buffers["temp_result"] = temp_buffer_2_double.data();
    } else if (task.task_type == "GEMM" && task.num_elements > 0) { // GEMM uses float
        // GEMM temp buffers might not be needed depending on the plan, but shown for completeness
        // For GEMM, num_elements might represent M*K for A, K*N for B, M*N for C.
        // This simple temp buffer allocation might not be sufficient for all GEMM plans.
    }


    for (const auto& step : plan.steps) {
        std::cout << "  -> Dispatching Step: " << step.operation_name << std::endl;

        if (step.operation_name == "JIT_COMPILE_SAXPY") {
            std::cout << "  -> [Cerebellum] Requesting JIT compilation for SAXPY..." << std::endl;
            last_jit_compiled_kernel_ = jit_engine_.compile_saxpy_for_data(task);
            // No actual execution for this step in terms of HAL call, it's a "transform"
        } else if (step.operation_name == "EXECUTE_JIT_SAXPY") {
            if (last_jit_compiled_kernel_) {
                std::cout << "  -> [Cerebellum] Executing JIT-compiled SAXPY kernel..." << std::endl;
                last_jit_compiled_kernel_();
            } else {
                std::cerr << "  -> [Cerebellum ERROR] EXECUTE_JIT_SAXPY called but no JIT kernel was compiled!" << std::endl;
                throw std::runtime_error("EXECUTE_JIT_SAXPY called without a compiled JIT kernel.");
            }
        } else if (kernel_lib_->count(step.operation_name)) { // Existing logic for standard kernels
            auto kernel_func = kernel_lib_->at(step.operation_name);

            // CONCEPTUAL KERNEL EXECUTION:
            // This is highly simplified. A real system needs to:
            // 1. Map string buffer IDs (e.g., "input", "temp_freq") to actual memory pointers.
            // 2. Cast these void* pointers to the correct types expected by the kernel.
            // 3. Pass all necessary arguments (alpha, M, N, K, etc.) to the kernel.
            // The current HAL::GenericKernel is void(), so it can't take args directly.
            // This would require a more complex HAL interface or per-kernel dispatch logic.

            // Example: If it was a SAXPY kernel (which it isn't with current GenericKernel)
            // if (step.operation_name == "SAXPY_OP_KEY") {
            //    auto saxpy_kernel = reinterpret_cast<void(*)(float, const float*, float*)>(kernel_func); // Risky cast
            //    float* in_ptr = static_cast<float*>(memory_buffers[step.input_buffer_id]);
            //    float* out_ptr = static_cast<float*>(memory_buffers[step.output_buffer_id]);
            //    // saxpy_kernel(alpha_value, in_ptr, out_ptr);
            // } else {
                 kernel_func(); // Execute the generic kernel from HAL as defined (void()).
            // }

        } else {
            throw std::runtime_error("Kernel not found in library: " + step.operation_name);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> latency_ns = end_time - start_time;

    // For this prototype, we'll equate flux with nanoseconds of latency.
    // A real system would have a more complex formula involving power, etc.
    double observed_flux = latency_ns.count();

    std::cout << "  ==> Execution Complete. Observed Flux (Latency ns): " << observed_flux << std::endl;
    return { observed_flux };
}

// Conceptual JIT engine logic
HAL::GenericKernel FluxJITEngine::compile_saxpy_for_data(VPU_Task& task) { // Modified signature
    std::cout << "    -> [JIT Engine] SAXPY compilation request for task_type: " << task.task_type << std::endl;

    float saxpy_param_a = 1.0f; // Default value
    if (task.specific_params.has_value()) {
        try {
            const VPU::SAXPYParams& params = std::any_cast<const VPU::SAXPYParams&>(task.specific_params);
            saxpy_param_a = params.a;
            std::cout << "    -> [JIT Engine] SAXPY 'a' parameter successfully extracted: " << saxpy_param_a << std::endl;
        } catch (const std::bad_any_cast& e) {
            std::cerr << "    -> [JIT Engine WARNING] Failed to cast specific_params to SAXPYParams: " << e.what()
                      << ". Using default 'a' = " << saxpy_param_a << std::endl;
        }
    } else {
        std::cout << "    -> [JIT Engine INFO] No specific_params set for SAXPY task. Using default 'a' = "
                  << saxpy_param_a << std::endl;
    }

    const float* x_data = static_cast<const float*>(task.data_in_a);
    std::vector<float> x_vec_analysis;
    if (x_data && task.num_elements > 0) {
       x_vec_analysis.assign(x_data, x_data + task.num_elements);
    }

    size_t zero_count = 0;
    if (!x_vec_analysis.empty()) {
        for (float val : x_vec_analysis) {
            if (val == 0.0f) {
                zero_count++;
            }
        }
    }
    double sparsity_ratio = x_vec_analysis.empty() ? 0.0 : static_cast<double>(zero_count) / x_vec_analysis.size();
    std::cout << "    -> [JIT Engine] SAXPY compilation request for a=" << saxpy_param_a
              << ". Data sparsity for input 'x': " << sparsity_ratio << std::endl;

    void* p_data_in_a = const_cast<void*>(task.data_in_a);
    void* p_data_out = task.data_out;
    size_t num_elements_captured = task.num_elements;

    if (sparsity_ratio > 0.5) {
        std::cout << "    -> [JIT Engine] Data is sparse. Providing 'SPARSE SAXPY' logic." << std::endl;
        return [saxpy_param_a, p_data_in_a, p_data_out, num_elements_captured](){ // Capture saxpy_param_a
            if (!p_data_in_a || !p_data_out || num_elements_captured == 0) {
                std::cerr << "JIT SPARSE KERNEL: Invalid data pointers or zero elements." << std::endl;
                return;
            }
            const float* x = static_cast<const float*>(p_data_in_a);
            float* y = static_cast<float*>(p_data_out);

            std::vector<float> x_temp(x, x + num_elements_captured);
            std::vector<float> y_temp(y, y + num_elements_captured);
            HAL::cpu_saxpy_sparse_specialized(saxpy_param_a, x_temp, y_temp); // Use saxpy_param_a
            for(size_t i=0; i<num_elements_captured; ++i) y[i] = y_temp[i];
        };
    } else {
        std::cout << "    -> [JIT Engine] Data is dense. Providing 'DENSE SAXPY' logic." << std::endl;
        return [saxpy_param_a, p_data_in_a, p_data_out, num_elements_captured](){ // Capture saxpy_param_a
            if (!p_data_in_a || !p_data_out || num_elements_captured == 0) {
                std::cerr << "JIT DENSE KERNEL: Invalid data pointers or zero elements." << std::endl;
                return;
            }
            const float* x = static_cast<const float*>(p_data_in_a);
            float* y = static_cast<float*>(p_data_out);

            std::vector<float> x_temp(x, x + num_elements_captured);
            std::vector<float> y_temp(y, y + num_elements_captured);
            HAL::cpu_saxpy_dense_specialized(saxpy_param_a, x_temp, y_temp); // Use saxpy_param_a
            for(size_t i=0; i<num_elements_captured; ++i) y[i] = y_temp[i];
        };
    }
}


} // namespace VPU
