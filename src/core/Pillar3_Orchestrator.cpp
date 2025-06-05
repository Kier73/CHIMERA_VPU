#include "core/Pillar3_Orchestrator.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace VPU {

Orchestrator::Orchestrator(std::shared_ptr<HardwareProfile> hw_profile) : hw_profile_(hw_profile) {
    if (!hw_profile_) {
        throw std::runtime_error("Orchestrator's HardwareProfile cannot be null.");
    }
}

// Main entry point for this pillar
ExecutionPlan Orchestrator::determine_optimal_path(const EnrichedExecutionContext& context) {
    std::cout << "[Pillar 3] Orchestrator: Making decision for task '" << context.task_type << "'..." << std::endl;

    // 1. Generate all possible ways to solve the problem
    auto candidates = generate_candidate_paths(context.task_type);
    if (candidates.empty()) {
        throw std::runtime_error("No candidate paths found for task: " + context.task_type);
    }

    // 2. Simulate the cost for each path based on the data profile
    double min_flux = -1.0;
    ExecutionPlan best_plan;

    for (auto& plan : candidates) {
        plan.predicted_holistic_flux = simulate_flux_cost(plan, *context.profile);
        std::cout << "  -> Path '" << plan.chosen_path_name << "' - Predicted Flux: " << plan.predicted_holistic_flux << std::endl;

        if (min_flux < 0 || plan.predicted_holistic_flux < min_flux) {
            min_flux = plan.predicted_holistic_flux;
            best_plan = plan;
        }
    }

    std::cout << "  ==> Decision: Chose '" << best_plan.chosen_path_name << "' with lowest predicted flux." << std::endl;
    return best_plan;
}


// A factory that creates potential strategies based on task type
std::vector<ExecutionPlan> Orchestrator::generate_candidate_paths(const std::string& task_type) {
    std::vector<ExecutionPlan> paths;
    if (task_type == "CONVOLUTION") {
        paths.push_back({"Time Domain (Direct)", 0.0, {
            {"CONV_DIRECT", "input", "output"}
        }});
        paths.push_back({"Frequency Domain (FFT)", 0.0, {
            {"FFT_FORWARD", "input", "temp_freq"},
            {"ELEMENT_WISE_MULTIPLY", "temp_freq", "temp_result"},
            {"FFT_INVERSE", "temp_result", "output"}
        }});
    } else if (task_type == "GEMM") {
        paths.push_back({"Naive GEMM", 0.0, {
            {"GEMM_NAIVE", "input", "output"}
        }});
        paths.push_back({"Flux-Adaptive GEMM", 0.0, {
            {"GEMM_FLUX_ADAPTIVE", "input", "output"}
        }});
    } else if (task_type == "SAXPY") {
        paths.push_back({"Standard SAXPY", 0.0, {
            {"SAXPY_STANDARD", "input", "output"}
        }});
        paths.push_back({"JIT Compiled SAXPY", 0.0, {
            {"JIT_COMPILE_SAXPY", "input_metadata", "compiled_kernel_id"}, // Conceptual step
            {"EXECUTE_JIT_SAXPY", "input", "output"}                        // Conceptual step
        }});
    }
    // TODO: Could add a "JIT Generation" path here for other ops.
    return paths;
}

// The predictive core of the VPU.
// Holistic_Flux = Σ(τ_transform) + τ_operation
// τ_operation = Base_Op_Cost + f(ACW, λ)
double Orchestrator::simulate_flux_cost(const ExecutionPlan& plan, const DataProfile& profile) {
    double total_flux = 0.0;

    // Sum of transform costs and the single final operation cost
    for(const auto& step : plan.steps) {
        // Is this a transform step?
        if (hw_profile_->transform_costs.count(step.operation_name)) {
            total_flux += hw_profile_->transform_costs.at(step.operation_name);
        }
        // Is this a final computation step?
        if(hw_profile_->base_operational_costs.count(step.operation_name)) {
            double base_op_cost = hw_profile_->base_operational_costs.at(step.operation_name);
            double dynamic_cost = 0.0;

            // This is f(ACW, λ) - the function that translates data complexity into cost
            if (step.operation_name == "CONV_DIRECT") {
                if (hw_profile_->flux_sensitivities.count("lambda_Conv_Amp") &&
                    hw_profile_->flux_sensitivities.count("lambda_Conv_Freq")) {
                    dynamic_cost = (profile.amplitude_flux * hw_profile_->flux_sensitivities.at("lambda_Conv_Amp")) +
                                   (profile.frequency_flux * hw_profile_->flux_sensitivities.at("lambda_Conv_Freq"));
                }
            } else if (step.operation_name == "GEMM_NAIVE" || step.operation_name == "GEMM_FLUX_ADAPTIVE") {
                if (hw_profile_->flux_sensitivities.count("lambda_Sparsity")) {
                    dynamic_cost = (1.0 - profile.sparsity_ratio) * hw_profile_->flux_sensitivities.at("lambda_Sparsity");
                }
            } else if (step.operation_name == "SAXPY_STANDARD") {
                if (hw_profile_->flux_sensitivities.count("lambda_SAXPY_generic")) {
                     dynamic_cost = profile.amplitude_flux * hw_profile_->flux_sensitivities.at("lambda_SAXPY_generic");
                }
            } else if (step.operation_name == "EXECUTE_JIT_SAXPY") {
                if (hw_profile_->flux_sensitivities.count("lambda_SAXPY_generic")) {
                    // JIT version might have different/lower dynamic cost sensitivity
                    dynamic_cost = profile.amplitude_flux * hw_profile_->flux_sensitivities.at("lambda_SAXPY_generic") * 0.5;
                }
            }
            // Note: ELEMENT_WISE_MULTIPLY currently has a base_operational_cost but no dynamic_cost logic.
            // This is acceptable for now as it's an intermediate step in FFT convolution.

            total_flux += (base_op_cost + dynamic_cost);
        }
        // The following 'else if' blocks for SAXPY_STANDARD and EXECUTE_JIT_SAXPY are removed
        // as their logic is now incorporated above within the main 'if(hw_profile_->base_operational_costs.count(step.operation_name))' block.
        // This simplifies the structure to:
        // 1. If transform_cost, add it.
        // 2. Else if base_operational_cost, calculate base + specific dynamic_cost, then add.
    }
    return total_flux;
}

} // namespace VPU
