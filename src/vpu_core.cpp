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


VPUCore::VPUCore() {
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
    ActualPerformanceRecord perf_record = pillar4_cerebellum_->execute(chosen_plan, task);

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
    pillar5_feedback_->learn_from_feedback(learning_ctx, chosen_plan.predicted_holistic_flux, perf_record);

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

    std::cout << "[VPUCore] Initial beliefs populated (with Pillar3/6 compatible costs)." << std::endl;
}

void VPUCore::initialize_hal() {
    kernel_lib_ = std::make_shared<HAL::KernelLibrary>();
    // In a real system, this might scan for plugins or pre-register kernels
    // For now, we can manually register a few.
    // Example: kernel_lib_->register_kernel("SAXPY_CPU", some_cpu_saxpy_function_ptr);
    std::cout << "[VPUCore] HAL and Kernel Library initialized." << std::endl;
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

} // namespace VPU
