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

    // 2. DECIDE: Use the Orchestrator to select the best execution plan.
    ExecutionPlan plan = pillar3_orchestrator_->determine_optimal_path(context);

    // 3. ACT: Use the Cerebellum to execute the plan and record performance.
    ActualPerformanceRecord perf_record = pillar4_cerebellum_->execute(plan, task);

    // 4. LEARN: Use the Feedback Loop to compare prediction and reality.
    LearningContext learning_ctx;
    learning_ctx.path_name = plan.chosen_path_name;

    bool is_transform_focused = false;

    if (plan.chosen_path_name.find("FFT") != std::string::npos) {
        learning_ctx.transform_key = "TRANSFORM_TIME_TO_FREQ";
        is_transform_focused = true;
    } else if (plan.chosen_path_name.find("JIT Compiled SAXPY") != std::string::npos) {
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
            for (const auto& step : plan.steps) {
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
    pillar5_feedback_->learn_from_feedback(learning_ctx, plan.predicted_holistic_flux, perf_record);
}

void VPUCore::initialize_beliefs() {
    hw_profile_ = std::make_shared<HardwareProfile>();
    // Populate with some baseline beliefs (costs)
    // These would typically be loaded from a config file or calibration routine

    // Example Beliefs for Operations (Cost per unit of Flux, arbitrary units)
    (*hw_profile_)["lambda_Conv_Amp"] = { {"CPU_generic", 1.0}, {"GPU_generic", 0.5} };
    (*hw_profile_)["lambda_Sparsity"] = { {"CPU_generic", 1.2}, {"GPU_generic", 0.6} };
    (*hw_profile_)["lambda_SAXPY_generic"] = { {"CPU_generic", 0.8}, {"GPU_generic", 0.3} };
     (*hw_profile_)["lambda_GEMM_ElementCount"] = { {"CPU_generic", 1.0} };


    // Example Beliefs for Transformations (Absolute cost in Flux units)
    (*hw_profile_)["TRANSFORM_TIME_TO_FREQ"] = { {"CPU_generic", 500.0}, {"GPU_generic", 100.0} };
    (*hw_profile_)["TRANSFORM_FREQ_TO_TIME"] = { {"CPU_generic", 450.0}, {"GPU_generic", 90.0} };
    (*hw_profile_)["TRANSFORM_JIT_COMPILE_SAXPY"] = { {"CPU_generic", 2000.0} }; // JIT typically CPU-bound for compilation

    std::cout << "[VPUCore] Initial beliefs populated." << std::endl;
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
        for (const auto& pair : *hw_profile_) {
            std::cout << "Metric/Transform: " << pair.first << std::endl;
            for (const auto& substrate_cost : pair.second) {
                std::cout << "  - Substrate: " << substrate_cost.first
                          << ", Cost: " << substrate_cost.second << std::endl;
            }
        }
    } else {
        std::cout << "No hardware profile loaded." << std::endl;
    }
    std::cout << "==============================================\n" << std::endl;
}

} // namespace VPU
