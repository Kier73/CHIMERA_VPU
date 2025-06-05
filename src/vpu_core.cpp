#include "vpu_core.h"
#include <iostream>
#include <string> // Required for std::string
#include <vector> // Required for std::vector in HAL calls
#include <memory> // Required for std::make_unique, std::make_shared

// Removed the local stub definition of VPU::Cortex from here.
// VPUCore will now use VPU::Cortex directly.

namespace VPU {

// --- Public API Implementation ---
VPU_Environment::VPU_Environment() : core(std::make_unique<VPUCore>()) {}
VPU_Environment::~VPU_Environment() {}
void VPU_Environment::execute(VPU_Task& task) { core->execute_task(task); }
void VPU_Environment::print_beliefs() { core->print_current_beliefs(); }


// --- VPUCore Implementation ---
VPUCore::VPUCore() {
    std::cout << "[VPU System] Core starting up..." << std::endl;

    // On initialization, all pillars are constructed and wired together.
    initialize_beliefs();
    initialize_hal();

    // Instantiate each pillar, providing shared access to necessary resources.
    pillar2_cortex_ = std::make_unique<VPU::Cortex>(); // Corrected instantiation
    pillar3_orchestrator_ = std::make_unique<Orchestrator>(hw_profile_);
    pillar4_cerebellum_ = std::make_unique<Cerebellum>(kernel_lib_);
    pillar5_feedback_ = std::make_unique<FeedbackLoop>(hw_profile_);

    std::cout << "[VPU System] All pillars are online. Ready." << std::endl;
}


// The full Perceive -> Decide -> Act -> Learn loop.
void VPUCore::execute_task(VPU_Task& task) {
    // 1. PERCEIVE: Use the Cortex to analyze the data.
    EnrichedExecutionContext context = pillar2_cortex_->analyze(task);
    // Direct use of RepresentationalFluxAnalyzer, profileOmni, or manual DataProfile creation is removed.

    // 2. DECIDE: Use the Orchestrator to select the best execution plan.
    ExecutionPlan plan = pillar3_orchestrator_->determine_optimal_path(context);

    // 3. ACT: Use the Cerebellum to execute the plan and record performance.
    ActualPerformanceRecord perf_record = pillar4_cerebellum_->execute(plan, task);

    // 4. LEARN: Use the Feedback Loop to compare prediction and reality.
    LearningContext learning_ctx;
    // Simple logic to create the learning context
    // This needs to be more robust and consider the actual plan steps.
    // For example, if the plan included an FFT, the transform_key should be set.
    // If the plan was a direct convolution, the operation_key related to CONV_DIRECT's sensitivity should be used.

    bool transform_involved = false;
    for(const auto& step : plan.steps) {
        if (hw_profile_->transform_costs.count(step.operation_name)) {
            learning_ctx.transform_key = step.operation_name; // e.g. TRANSFORM_TIME_TO_FREQ
            transform_involved = true;
            break; // Assume one primary transform for simplicity in this context assignment
        }
    }

    if (!transform_involved) {
        // If no transform, the main operation of the plan dictates the sensitivity key.
        // Find the first operational step in the plan to determine the key.
        for(const auto& step : plan.steps) {
            if (hw_profile_->base_operational_costs.count(step.operation_name)) {
                // Map operation name to sensitivity key. This is a heuristic.
                // A more robust mapping might be needed.
                if (step.operation_name == "CONV_DIRECT") {
                    learning_ctx.operation_key = "lambda_A";
                } else if (step.operation_name == "GEMM_NAIVE" || step.operation_name == "GEMM_FLUX_ADAPTIVE") {
                    learning_ctx.operation_key = "lambda_Sparsity";
                } else if (step.operation_name == "ELEMENT_WISE_MULTIPLY" && plan.chosen_path_name.find("FFT") != std::string::npos) {
                    // This is part of an FFT path, but the transform cost itself is primary.
                    // However, if TRANSFORM_TIME_TO_FREQ wasn't caught, this might be a fallback.
                    // Or, ELEMENT_WISE_MULTIPLY could have its own lambda if it's flux-sensitive.
                    // For now, this case is tricky. If transform_key is set, it takes precedence.
                }
                // Add other mappings as necessary
                if (!learning_ctx.operation_key.empty()) break;
            }
        }
    }


    pillar5_feedback_->learn_from_feedback(learning_ctx, plan.predicted_holistic_flux, perf_record);
}

void VPUCore::initialize_beliefs() {
    hw_profile_ = std::make_shared<HardwareProfile>();
    // Load initial beliefs (our "common sense" priors). These will be refined over time.
    hw_profile_->base_operational_costs = {
        {"CONV_DIRECT", 50000.0},
        {"ELEMENT_WISE_MULTIPLY", 10000.0}, // Part of the FFT path
        {"GEMM_NAIVE", 100000.0},
        {"GEMM_FLUX_ADAPTIVE", 300000.0}  // Higher base cost due to setup overhead
    };
    hw_profile_->transform_costs = {
        {"TRANSFORM_TIME_TO_FREQ", 200000.0}, // Cost for one direction (e.g. R2C)
        {"FFT_FORWARD", 200000.0}, // Explicitly for FFT forward
        {"FFT_INVERSE", 200000.0}  // Explicitly for FFT inverse
    };
    hw_profile_->flux_sensitivities = {
        {"lambda_A", 100.0},          // Sensitivity to amplitude flux for CONV
        {"lambda_Sparsity", 5000000.0}  // Sensitivity to density for GEMM
    };
}

void VPUCore::initialize_hal() {
    kernel_lib_ = std::make_shared<HAL::KernelLibrary>();
    // These would be bound to actual data from VPU_Task in Cerebellum::execute
    // For now, they are parameterless lambdas as per HAL::GenericKernel type.
    // The actual data handling is conceptual in Cerebellum.
    // Dummy const vectors for input arguments where needed by HAL function signatures.
    const std::vector<double> const_dummy_double_in_vec;
    const std::vector<float> const_dummy_float_in_vec;

    (*kernel_lib_)["CONV_DIRECT"] = [](){
        std::cout << "    -> [HAL KERNEL STUB] CONV_DIRECT called." << std::endl;
        // HAL::cpu_conv_direct(params...);
    };
    (*kernel_lib_)["FFT_FORWARD"] = [const_dummy_double_in_vec](){
        std::vector<double> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_fft_forward(const_dummy_double_in_vec, dummy_out_vec);
    };
    (*kernel_lib_)["ELEMENT_WISE_MULTIPLY"] = [](){
        std::cout << "    -> [HAL KERNEL STUB] ELEMENT_WISE_MULTIPLY called." << std::endl;
        // HAL::cpu_element_wise_multiply(params...);
    };
    (*kernel_lib_)["FFT_INVERSE"] = [const_dummy_double_in_vec](){
        std::vector<double> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_fft_inverse(const_dummy_double_in_vec, dummy_out_vec);
    };
    (*kernel_lib_)["GEMM_NAIVE"] = [const_dummy_float_in_vec](){
        std::vector<float> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_gemm_naive(const_dummy_float_in_vec, const_dummy_float_in_vec, dummy_out_vec, 0,0,0);
    };
    (*kernel_lib_)["GEMM_FLUX_ADAPTIVE"] = [const_dummy_float_in_vec](){
        std::vector<float> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_gemm_flux_adaptive(const_dummy_float_in_vec, const_dummy_float_in_vec, dummy_out_vec, 0,0,0);
    };

}


void VPUCore::print_current_beliefs() {
    std::cout << "\n===== VPU CURRENT BELIEFS =====\n";
    std::cout << "--- Base Operational Costs ---\n";
    for(const auto& [key, val] : hw_profile_->base_operational_costs) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
    std::cout << "--- Transform Costs ---\n";
    for(const auto& [key, val] : hw_profile_->transform_costs) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
    std::cout << "--- Flux Sensitivities ---\n";
    for(const auto& [key, val] : hw_profile_->flux_sensitivities) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
     std::cout << "==============================\n" << std::endl;
}

} // namespace VPU
