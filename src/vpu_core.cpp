#include "vpu_core.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Ensure Pillar1_Synapse.h is included via vpu_core.h or directly if needed.
// It should be included by vpu_core.h already.

namespace VPU {

// VPU_Environment methods (assuming they are defined elsewhere or here)
// If VPU_Environment methods are in this file, they would typically be:
VPU_Environment::VPU_Environment() : core(std::make_unique<VPUCore>()) {
    std::cout << "[VPU_Environment] Created and VPUCore initialized." << std::endl;
}

VPU_Environment::~VPU_Environment() {
    std::cout << "[VPU_Environment] Destroyed." << std::endl;
}

void VPU_Environment::execute(VPU_Task& task) {
    if (core) {
        core->execute_task(task);
    } else {
        std::cerr << "[VPU_Environment] Error: VPUCore not initialized." << std::endl;
    }
}

const ActualPerformanceRecord& VPU_Environment::get_last_performance_record() const {
    if (core) {
        return core->get_last_performance_record();
    } else {
        std::cerr << "[VPU_Environment] Error: VPUCore not initialized. Returning default/empty ActualPerformanceRecord." << std::endl;
        // This is problematic as we must return a reference.
        // Consider throwing an exception or having a static default instance.
        // For now, this path indicates a programming error if core is null.
        static const ActualPerformanceRecord default_record = {}; // Default empty record
        return default_record;
    }
}

void VPU_Environment::print_beliefs() {
    if (core) {
        core->print_current_beliefs();
    } else {
        std::cerr << "[VPU_Environment] Error: VPUCore not initialized." << std::endl;
    }
}

// Test helper method implementation
VPUCore* VPU_Environment::get_core_for_testing() {
    return core.get();
}


VPUCore::VPUCore() : last_perf_record_() { // Initialize last_perf_record_
    std::cout << "[VPU System] Core starting up..." << std::endl;
    initialize_beliefs();
    initialize_hal();

    // Instantiate each pillar, providing shared access to necessary resources.
    pillar1_synapse_ = std::make_unique<Pillar1_Synapse>(); // Added
    pillar2_cortex_ = std::make_unique<VPU::Cortex>();
    pillar3_orchestrator_ = std::make_unique<Orchestrator>(hw_profile_);
    pillar4_cerebellum_ = std::make_unique<Cerebellum>(kernel_lib_);
    pillar5_feedback_ = std::make_unique<FeedbackLoop>(hw_profile_);
    // Initialize Pillar 6, ensuring hw_profile_ and kernel_lib_ are already initialized
    pillar6_task_graph_orchestrator_ = std::make_unique<TaskGraphOrchestrator>(kernel_lib_, hw_profile_);


    std::cout << "[VPU System] All pillars are online. Ready." << std::endl;
}

void VPUCore::execute_task(VPU_Task& task) {
    // 0. SUBMIT & VALIDATE: Pass task through Pillar1 for initial intake.
    std::cout << "[VPUCore] Submitting task ID: " << task.task_id << " to Pillar1_Synapse." << std::endl;
    if (!pillar1_synapse_->submit_task(task)) {
        std::cerr << "[VPUCore] Task ID: " << task.task_id << " rejected by Pillar1_Synapse. Aborting execution." << std::endl;
        return; // Task failed initial validation or processing in Pillar1
    }
    std::cout << "[VPUCore] Task ID: " << task.task_id << " successfully processed by Pillar1_Synapse." << std::endl;

    // 1. PERCEIVE: Use the Cortex to analyze the data.
    EnrichedExecutionContext context = pillar2_cortex_->analyze(task);

    // 2. DECIDE: Use the Orchestrator to get candidate execution plans.
    std::vector<ExecutionPlan> candidate_plans = pillar3_orchestrator_->determine_optimal_path(context);

    if (candidate_plans.empty()) {
        std::cerr << "[VPUCore] Error: Orchestrator returned no candidate plans for task ID: " << task.task_id << ". Aborting." << std::endl;
        // Optionally, set task status to error
        return;
    }

    ExecutionPlan chosen_plan = candidate_plans.front(); // Default to the best plan
    bool explored = false;

    if (pillar5_feedback_->should_explore()) {
        if (candidate_plans.size() > 1) {
            // Simple exploration: choose the second-best plan if available.
            // More sophisticated strategies could be: random choice from non-optimal,
            // or a plan with specific characteristics to test.
            chosen_plan = candidate_plans[1];
            explored = true;
            std::cout << "[VPUCore] EXPLORATION: Chose suboptimal plan '" << chosen_plan.chosen_path_name
                      << "' (Predicted Flux: " << chosen_plan.predicted_holistic_flux
                      << ") instead of optimal '" << candidate_plans.front().chosen_path_name
                      << "' (Predicted Flux: " << candidate_plans.front().predicted_holistic_flux
                      << ")" << std::endl;
        } else {
            std::cout << "[VPUCore] EXPLORATION desired, but no alternative paths available for task ID: " << task.task_id << "." << std::endl;
        }
    } else {
         std::cout << "[VPUCore] Chose optimal plan '" << chosen_plan.chosen_path_name << "' with predicted flux " << chosen_plan.predicted_holistic_flux << "." << std::endl;
    }

    // 3. ACT: Use the Cerebellum to execute the chosen plan and record performance.
    // Store the performance record
    last_perf_record_ = pillar4_cerebellum_->execute(chosen_plan, task);

    // 4. LEARN: Use the Feedback Loop to compare prediction and reality.
    // Crucially, use the chosen_plan's name and its predicted_holistic_flux for learning.
    LearningContext learning_ctx;
    learning_ctx.path_name = chosen_plan.chosen_path_name;
    if (explored) {
        learning_ctx.path_name += " (Exploratory)"; // Mark exploratory paths in context
    }


    bool is_transform_focused = false;

    // Corrected: Use chosen_plan for LearningContext creation
    if (chosen_plan.chosen_path_name.find("FFT") != std::string::npos) {
        learning_ctx.transform_key = "TRANSFORM_TIME_TO_FREQ";
        is_transform_focused = true;
    } else if (chosen_plan.chosen_path_name.find("JIT Compiled SAXPY") != std::string::npos) {
        learning_ctx.transform_key = "TRANSFORM_JIT_COMPILE_SAXPY";
        learning_ctx.main_operation_name = "EXECUTE_JIT_SAXPY";
        learning_ctx.operation_key = "lambda_SAXPY_generic";
        is_transform_focused = true;
    }

    if (!is_transform_focused || !learning_ctx.main_operation_name.empty()) {
        if (task.task_type == "CONVOLUTION") {
            if (!is_transform_focused) {
                learning_ctx.main_operation_name = "CONV_DIRECT";
                learning_ctx.operation_key = "lambda_Conv_Amp";
            }
        } else if (task.task_type == "GEMM") {
            // Corrected: Use chosen_plan for LearningContext creation
            for (const auto& step : chosen_plan.steps) {
                if (step.operation_name == "GEMM_NAIVE" || step.operation_name == "GEMM_FLUX_ADAPTIVE") {
                    learning_ctx.main_operation_name = step.operation_name;
                    break;
                }
            }
            learning_ctx.operation_key = "lambda_Sparsity";
        } else if (task.task_type == "SAXPY") {
            if (!is_transform_focused) {
                 learning_ctx.main_operation_name = "SAXPY_STANDARD";
                 learning_ctx.operation_key = "lambda_SAXPY_generic";
            }
        }
    }
    // Pass the predicted flux of the *actually executed plan* to learn_from_feedback
    pillar5_feedback_->learn_from_feedback(learning_ctx, chosen_plan.predicted_holistic_flux, last_perf_record_);

    // 5. RECORD & ADAPT (Pillar 6): Record the executed plan for graph analysis and potential fusion.
    // The analyze_and_fuse_patterns() is called periodically from within record_executed_plan().
    if (pillar6_task_graph_orchestrator_) {
        pillar6_task_graph_orchestrator_->record_executed_plan(chosen_plan);
    }
}

void VPUCore::initialize_beliefs() {
    hw_profile_ = std::make_shared<HardwareProfile>();
    // Populate with some baseline beliefs (costs)
    // These would typically be loaded from a config file or calibration routine

    // Example Beliefs for Operations (Cost per unit of Flux, arbitrary units)
    // This is the old way, keeping it for now if anything relies on this exact structure.
    // (*hw_profile_)["lambda_Conv_Amp"] = { {"CPU_generic", 1.0}, {"GPU_generic", 0.5} };
    // (*hw_profile_)["lambda_Sparsity"] = { {"CPU_generic", 1.2}, {"GPU_generic", 0.6} };

    // New way for Pillar 3 & 6 compatibility (flat map for base_operational_costs & transform_costs)
    // These are conceptual base costs for operations if they were run on a generic CPU.
    hw_profile_->base_operational_costs["CONV_DIRECT"] = 200.0;
    hw_profile_->base_operational_costs["ELEMENT_WISE_MULTIPLY"] = 50.0;
    hw_profile_->base_operational_costs["GEMM_NAIVE"] = 500.0;
    hw_profile_->base_operational_costs["GEMM_FLUX_ADAPTIVE"] = 450.0;
    hw_profile_->base_operational_costs["SAXPY_STANDARD"] = 100.0;
    hw_profile_->base_operational_costs["EXECUTE_JIT_SAXPY"] = 70.0; // Cost of executing a JITted kernel

    // Sensitivities (as used by Pillar 3)
    hw_profile_->flux_sensitivities["lambda_Conv_Amp"] = 1.0;
    hw_profile_->flux_sensitivities["lambda_Conv_Freq"] = 0.8;
    hw_profile_->flux_sensitivities["lambda_Sparsity"] = 150.0; // Higher impact for sparsity
    hw_profile_->flux_sensitivities["lambda_SAXPY_generic"] = 0.5;

    // Example Beliefs for Transformations (Absolute cost in Flux units)
    hw_profile_->transform_costs["FFT_FORWARD"] = 300.0;
    hw_profile_->transform_costs["FFT_INVERSE"] = 280.0;
    hw_profile_->transform_costs["JIT_COMPILE_SAXPY"] = 1000.0; // Cost of the JIT compilation step itself

    // New Hamming Weight sensitivities
    hw_profile_->flux_sensitivities["SAXPY_STANDARD_lambda_hw_combined"] = 0.1;    // Default sensitivity
    hw_profile_->flux_sensitivities["EXECUTE_JIT_SAXPY_lambda_hw_combined"] = 0.05; // JIT might be less sensitive to input HW
    hw_profile_->flux_sensitivities["GEMM_NAIVE_lambda_hw_combined"] = 0.2;
    hw_profile_->flux_sensitivities["GEMM_FLUX_ADAPTIVE_lambda_hw_combined"] = 0.15;
    hw_profile_->flux_sensitivities["CONV_DIRECT_lambda_hw_combined"] = 0.25;
    // ELEMENT_WISE_MULTIPLY might also have one if it's made data-dependent beyond base cost
    // hw_profile_->flux_sensitivities["ELEMENT_WISE_MULTIPLY_lambda_hw_combined"] = 0.05;


    std::cout << "[VPUCore] Initial beliefs populated (with Pillar3/6 compatible costs)." << std::endl;
}

#include "hal/hal_utils.h" // For VPU::HAL::calculate_data_hamming_weight
#include "hal/hal.h"       // For VPU::HAL::cpu_saxpy etc. (already included via vpu_core.h usually)

void VPUCore::initialize_hal() {
    kernel_lib_ = std::make_shared<HAL::KernelLibrary>();

    // SAXPY_STANDARD Kernel
    (*kernel_lib_)["SAXPY_STANDARD"] = [](VPU_Task& task) -> HAL::KernelFluxReport {
        HAL::KernelFluxReport report;
        if (!task.data_in_a || !task.data_out || task.num_elements == 0) {
            std::cerr << "SAXPY_STANDARD: Invalid data pointers or zero elements." << std::endl;
            return {0,0,0};
        }
        const float* x_ptr = static_cast<const float*>(task.data_in_a);
        // Assuming task.data_in_b might hold 'y' for SAXPY if 'a' is in task.alpha
        // Or, more likely, task.data_out is also the input 'y' for SAXPY (in-place like)
        // For this example, let's assume data_out is y and is modified in place.
        // And task.data_in_a is x.
        float* y_ptr = static_cast<float*>(task.data_out);

        std::vector<float> x_vec(x_ptr, x_ptr + task.num_elements);
        std::vector<float> y_vec(y_ptr, y_ptr + task.num_elements); // y is input/output

        report.hw_in_cost = HAL::calculate_data_hamming_weight(x_vec.data(), x_vec.size() * sizeof(float));
        report.hw_in_cost += HAL::calculate_data_hamming_weight(y_vec.data(), y_vec.size() * sizeof(float)); // Initial Y

        HAL::cpu_saxpy(task.alpha, x_vec, y_vec); // y_vec is updated here

        // Copy data back from y_vec to task.data_out if necessary (it is, as y_vec is a copy)
        for(size_t i = 0; i < task.num_elements; ++i) y_ptr[i] = y_vec[i];

        report.hw_out_cost = HAL::calculate_data_hamming_weight(y_vec.data(), y_vec.size() * sizeof(float));
        report.cycle_cost = task.num_elements * 2; // 1 mul, 1 add
        return report;
    };

    // GEMM_NAIVE Kernel
    (*kernel_lib_)["GEMM_NAIVE"] = [](VPU_Task& task) -> HAL::KernelFluxReport {
        HAL::KernelFluxReport report;
        // Assuming M, N, K are packed into VPU_Task extended_params or similar
        // For simplicity, let's assume num_elements means M*K for A, and K*N for B, M*N for C
        // This is a gross simplification. A real system needs proper dimension handling.
        if (!task.data_in_a || !task.data_in_b || !task.data_out ||
            !task.extended_params.count("M") || !task.extended_params.count("N") || !task.extended_params.count("K")) {
            std::cerr << "GEMM_NAIVE: Invalid data pointers or missing M, N, K dimensions." << std::endl;
            return {0,0,0};
        }
        int M = task.extended_params["M"];
        int N = task.extended_params["N"];
        int K = task.extended_params["K"];

        const float* A_ptr = static_cast<const float*>(task.data_in_a);
        const float* B_ptr = static_cast<const float*>(task.data_in_b);
        float* C_ptr = static_cast<float*>(task.data_out);

        std::vector<float> A_vec(A_ptr, A_ptr + (M * K));
        std::vector<float> B_vec(B_ptr, B_ptr + (K * N));
        std::vector<float> C_vec(M * N); // Output buffer, HAL::cpu_gemm_naive will fill it

        report.hw_in_cost = HAL::calculate_data_hamming_weight(A_vec.data(), A_vec.size() * sizeof(float));
        report.hw_in_cost += HAL::calculate_data_hamming_weight(B_vec.data(), B_vec.size() * sizeof(float));

        HAL::cpu_gemm_naive(A_vec, B_vec, C_vec, M, N, K);

        for(size_t i = 0; i < (size_t)(M*N); ++i) C_ptr[i] = C_vec[i]; // Copy out

        report.hw_out_cost = HAL::calculate_data_hamming_weight(C_vec.data(), C_vec.size() * sizeof(float));
        report.cycle_cost = M * N * K * 2; // Roughly M*N*K multiply-adds
        return report;
    };

    // FFT_FORWARD Kernel (Double precision)
    (*kernel_lib_)["FFT_FORWARD"] = [](VPU_Task& task) -> HAL::KernelFluxReport {
        HAL::KernelFluxReport report;
        if (!task.data_in_a || !task.data_out || task.num_elements == 0) {
            std::cerr << "FFT_FORWARD: Invalid data pointers or zero elements." << std::endl;
            return {0,0,0};
        }
        const double* in_ptr = static_cast<const double*>(task.data_in_a);
        double* out_ptr = static_cast<double*>(task.data_out); // Output buffer for FFT

        std::vector<double> in_vec(in_ptr, in_ptr + task.num_elements);
        // FFT output size is typically N/2+1 complex, but hal function takes N doubles for out
        // For simplicity, assume HAL::cpu_fft_forward handles output vector sizing or expects it pre-sized.
        // Let's assume output is also task.num_elements for this HAL function.
        std::vector<double> out_vec(task.num_elements);


        report.hw_in_cost = HAL::calculate_data_hamming_weight(in_vec.data(), in_vec.size() * sizeof(double));

        HAL::cpu_fft_forward(in_vec, out_vec);

        for(size_t i=0; i<task.num_elements; ++i) out_ptr[i] = out_vec[i];


        report.hw_out_cost = HAL::calculate_data_hamming_weight(out_vec.data(), out_vec.size() * sizeof(double));
        // Cycle cost for FFT is roughly N log N
        if (task.num_elements > 0) {
            report.cycle_cost = static_cast<uint64_t>(task.num_elements * std::log2(task.num_elements) * 5); // *5 as a scaling factor
        } else {
            report.cycle_cost = 0;
        }
        return report;
    };

    // TODO: Add other kernels like FFT_INVERSE, GEMM_FLUX_ADAPTIVE, CONV_DIRECT etc.

    std::cout << "[VPUCore] HAL and Kernel Library initialized with new flux-reporting kernels." << std::endl;
}

void VPUCore::print_current_beliefs() {
    std::cout << "\n===== VPU Current Beliefs (Hardware Profile) =====" << std::endl;
    if (hw_profile_) {
        std::cout << "Base Operational Costs:" << std::endl;
        for (const auto& pair : hw_profile_->base_operational_costs) {
            std::cout << "  - Op: " << pair.first << ", Cost: " << pair.second << std::endl;
        }
        std::cout << "Transform Costs:" << std::endl;
        for (const auto& pair : hw_profile_->transform_costs) {
            std::cout << "  - Transform: " << pair.first << ", Cost: " << pair.second << std::endl;
        }
        std::cout << "Flux Sensitivities (Lambdas):" << std::endl;
        for (const auto& pair : hw_profile_->flux_sensitivities) {
            std::cout << "  - Lambda: " << pair.first << ", Value: " << pair.second << std::endl;
        }
        // This part is for the old structure, can be removed if fully migrated
        // std::cout << "Raw Hardware Profile Data (if any using old structure):" << std::endl;
        // for (const auto& pair : *hw_profile_) { // Assuming hw_profile_ is still the map itself for this part
        //     if (hw_profile_->base_operational_costs.find(pair.first) == hw_profile_->base_operational_costs.end() &&
        //         hw_profile_->transform_costs.find(pair.first) == hw_profile_->transform_costs.end() &&
        //         hw_profile_->flux_sensitivities.find(pair.first) == hw_profile_->flux_sensitivities.end()) {
        //         std::cout << "Metric/Transform (Old Style): " << pair.first << std::endl;
        //         // for (const auto& substrate_cost : pair.second) { // This line would error as pair.second is not a map
        //         //     std::cout << "  - Substrate: " << substrate_cost.first
        //         //               << ", Cost: " << substrate_cost.second << std::endl;
        //         // }
        //     }
        // }
    } else {
        std::cout << "No hardware profile loaded." << std::endl;
    }
    std::cout << "==============================================\n" << std::endl;
}

const ActualPerformanceRecord& VPUCore::get_last_performance_record() const {
    return last_perf_record_;
}

} // namespace VPU
