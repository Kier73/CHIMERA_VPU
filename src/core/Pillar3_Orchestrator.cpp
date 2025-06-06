#include "core/Pillar3_Orchestrator.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace VPU {

Orchestrator::Orchestrator(std::shared_ptr<HardwareProfile> hw_profile) : hw_profile_(hw_profile), use_llm_for_paths_(false) {
    if (!hw_profile_) {
        throw std::runtime_error("Orchestrator's HardwareProfile cannot be null.");
    }
}

// Main entry point for this pillar
std::vector<ExecutionPlan> Orchestrator::determine_optimal_path(const EnrichedExecutionContext& context) { // Changed return type
    std::cout << "[Pillar 3] Orchestrator: Determining candidate paths for task '" << context.task_type << "'..." << std::endl;

    std::vector<ExecutionPlan> candidates;
    if (use_llm_for_paths_) {
        std::cout << "[Pillar 3] Orchestrator: Using LLM for path generation." << std::endl;
        candidates = generate_paths_with_llm(context);
        // Fallback or combine with traditional method if LLM returns no paths or if desired
        if (candidates.empty()) {
            std::cout << "[Pillar 3] Orchestrator: LLM returned no paths, falling back to traditional method." << std::endl;
            candidates = generate_candidate_paths(context.task_type);
        }
    } else {
        // 1. Generate all possible ways to solve the problem
        candidates = generate_candidate_paths(context.task_type);
    }

    if (candidates.empty()) {
        throw std::runtime_error("No candidate paths found for task: " + context.task_type);
    }

    // 2. Simulate the cost for each path based on the data profile
    std::cout << "[Pillar 3] Orchestrator: Simulating costs for " << candidates.size() << " candidate path(s)..." << std::endl;
    for (auto& plan : candidates) {
        plan.predicted_holistic_flux = simulate_flux_cost(plan, *context.profile);
        std::cout << "  -> Path '" << plan.chosen_path_name << "' - Predicted Flux: " << plan.predicted_holistic_flux << std::endl;
    }

    // 3. Sort candidates by predicted_holistic_flux (ascending)
    std::sort(candidates.begin(), candidates.end(), [](const ExecutionPlan& a, const ExecutionPlan& b) {
        return a.predicted_holistic_flux < b.predicted_holistic_flux;
    });

    std::cout << "[Pillar 3] Orchestrator: Returning " << candidates.size() << " candidate path(s) sorted by predicted flux." << std::endl;
    if (!candidates.empty()) {
        std::cout << "  -> Top candidate: '" << candidates.front().chosen_path_name << "' with flux " << candidates.front().predicted_holistic_flux << std::endl;
    }
    return candidates;
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

    // --- Apply IoT Sensor Data Adjustments ---
    double original_flux = total_flux;
    double cost_multiplier = 1.0;
    std::string log_iot_adjustments = "";

    // Example: Adjust for temperature
    // Thresholds and multipliers are examples and would ideally be configurable or learned.
    const double TEMP_THRESHOLD_HIGH = 85.0;
    const double TEMP_MULTIPLIER_HIGH = 1.5;
    if (profile.temperature_celsius > TEMP_THRESHOLD_HIGH) {
        cost_multiplier *= TEMP_MULTIPLIER_HIGH;
        log_iot_adjustments += "TempHigh(" + std::to_string(profile.temperature_celsius) + "C * " + std::to_string(TEMP_MULTIPLIER_HIGH) + "); ";
    }

    // Example: Adjust for power draw (scaling cost)
    const double POWER_THRESHOLD_WARN = 100.0; // Watts
    const double POWER_SCALE_FACTOR = 0.005; // 0.5% cost increase per Watt over threshold
    if (profile.power_draw_watts > POWER_THRESHOLD_WARN) {
        double excess_power_penalty = (profile.power_draw_watts - POWER_THRESHOLD_WARN) * POWER_SCALE_FACTOR;
        cost_multiplier *= (1.0 + excess_power_penalty);
        log_iot_adjustments += "ExcessPwr(" + std::to_string(profile.power_draw_watts) + "W -> penalty " + std::to_string(excess_power_penalty) + "); ";
    }

    // Example: Adjust for network latency (if plan involves network - simplistic check)
    // A more robust check would inspect plan.steps for network-bound operations.
    const double NET_LATENCY_THRESHOLD_MS = 100.0;
    const double NET_LATENCY_MULTIPLIER = 1.2;
    bool plan_uses_network = false; // Simple placeholder; a real check would analyze plan steps
    for(const auto& step : plan.steps) {
        // Conceptual: Check if step.operation_name implies network usage
        if (step.operation_name.find("NETWORK_") != std::string::npos || step.operation_name.find("REMOTE_") != std::string::npos) {
            plan_uses_network = true;
            break;
        }
    }
    if (plan_uses_network && profile.network_latency_ms > NET_LATENCY_THRESHOLD_MS) {
        cost_multiplier *= NET_LATENCY_MULTIPLIER;
        log_iot_adjustments += "NetLatency(" + std::to_string(profile.network_latency_ms) + "ms * " + std::to_string(NET_LATENCY_MULTIPLIER) + "); ";
    }

    // Example: Adjust for I/O throughput (if plan involves heavy I/O)
    // This is highly conceptual as specific I/O steps aren't well-defined yet.
    const double IO_THROUGHPUT_LOW_MBPS = 50.0; // If throughput is below this, penalize.
    const double IO_THROUGHPUT_MULTIPLIER = 1.15;
    bool plan_uses_heavy_io = false; // Simple placeholder
     for(const auto& step : plan.steps) {
        if (step.operation_name.find("DISK_") != std::string::npos || step.operation_name.find("LOAD_") != std::string::npos) {
            plan_uses_heavy_io = true;
            break;
        }
    }
    if (plan_uses_heavy_io && profile.io_throughput_mbps < IO_THROUGHPUT_LOW_MBPS && profile.io_throughput_mbps > 0) { // avoid division by zero if 0 is possible
        cost_multiplier *= IO_THROUGHPUT_MULTIPLIER;
        log_iot_adjustments += "LowIO(" + std::to_string(profile.io_throughput_mbps) + "Mbps * " + std::to_string(IO_THROUGHPUT_MULTIPLIER) + "); ";
    }


    // Example: Adjust for data quality (inverse relationship: higher quality = lower effective cost)
    if (profile.data_quality_score > 0.0 && profile.data_quality_score < 1.0) { // Avoid division by zero or no effect if 1.0
        cost_multiplier /= profile.data_quality_score; // Lower quality (e.g. 0.8) increases cost_multiplier
        log_iot_adjustments += "DataQuality(" + std::to_string(profile.data_quality_score) + " score / " + std::to_string(cost_multiplier) + "); ";
    } else if (profile.data_quality_score <= 0.0) { // Handle bad quality score explicitly
        cost_multiplier *= 10.0; // Arbitrary large penalty for very bad/zero quality
        log_iot_adjustments += "BadDataQuality(" + std::to_string(profile.data_quality_score) + " -> massive penalty); ";
    }


    total_flux *= cost_multiplier;

    if (original_flux != total_flux) {
        std::cout << "      [Pillar 3] Flux for plan '" << plan.chosen_path_name << "' adjusted by IoT factors: "
                  << original_flux << " -> " << total_flux
                  << ". Adjustments: " << (log_iot_adjustments.empty() ? "None" : log_iot_adjustments)
                  << std::endl;
    }
    // --- End of IoT Sensor Data Adjustments ---

    return total_flux;
}

void Orchestrator::set_llm_path_generation(bool enable) {
    use_llm_for_paths_ = enable;
    std::cout << "[Pillar 3] Orchestrator: LLM path generation " << (enable ? "enabled." : "disabled.") << std::endl;
}

std::vector<ExecutionPlan> Orchestrator::generate_paths_with_llm(const EnrichedExecutionContext& context) {
    std::cout << "[Pillar 3] Orchestrator: LLM path generation called with context for task type: " << context.task_type << std::endl;
    // In a real scenario, this would involve formatting the context and profile,
    // sending it to an LLM, and parsing the response.
    // For now, returning an empty vector to signify conceptual implementation.
    // Or, return a simple hardcoded plan for basic testing:
    /*
    if (context.task_type == "CONVOLUTION") {
        return {{"LLM Suggested Path (CONV)", 0.0, {{"LLM_OP_CONV", "input", "output"}}}};
    }
    */
    return {}; // Returning empty for now
}

} // namespace VPU
