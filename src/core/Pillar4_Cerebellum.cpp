#include "core/Pillar4_Cerebellum.h"
#include <chrono>
#include <stdexcept> // Required for std::runtime_error
#include <iostream>  // Required for std::cout

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

        // This is where JIT compilation would be invoked if the plan demanded it.
        // if (step.operation_name == "JIT_GENERATE_SAXPY") {
        //     // Assuming task.data_in_a is std::vector<float> for this conceptual JIT
        //     if (task.data_in_a) { // Basic check
        //          const std::vector<float>* data_for_jit = static_cast<const std::vector<float>*>(task.data_in_a);
        //          auto specialized_kernel = jit_engine_.compile_saxpy_for_data(*data_for_jit);
        //          if(specialized_kernel) { specialized_kernel(); }
        //          else { /* fall back to HAL kernel */ }
        //     }
        // }


        if (kernel_lib_->count(step.operation_name)) {
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
HAL::GenericKernel FluxJITEngine::compile_saxpy_for_data(const std::vector<float>& data) {
    std::cout << "    -> [JIT Engine] Received compilation request for SAXPY." << std::endl;
    // 1. Analyze data for a unique optimization opportunity (e.g., extreme sparsity).
    // Example: check if a large percentage of data is zero.
    size_t zero_count = 0;
    for(float val : data) {
        if (val == 0.0f) {
            zero_count++;
        }
    }
    bool is_worthwhile = (static_cast<double>(zero_count) / data.size() > 0.8); // e.g. >80% sparse

    if (is_worthwhile) {
        std::cout << "    -> [JIT Engine] Data is highly sparse. Compiling specialized 'NNZ' (non-zero) SAXPY kernel." << std::endl;
        // 2. Use LLVM (conceptually) to generate a new function in memory.
        // This new function would iterate only over non-zero elements.
        // 3. Return a function pointer (as std::function) to the new kernel.
        return [](){ std::cout << "    -> [JIT KERNEL] Executing specialized JIT-generated SAXPY kernel for sparse data." << std::endl; /* Actual JIT'd code would run here */ };
    } else {
        std::cout << "    -> [JIT Engine] Data is dense enough. SAXPY compilation not worthwhile. Declining." << std::endl;
        return nullptr; // Indicate that a fallback to a standard HAL kernel is needed.
    }
}


} // namespace VPU
