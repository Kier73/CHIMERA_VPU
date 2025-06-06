#include "core/Pillar6_TaskGraphOrchestrator.h"
#include <iostream> // For logging

namespace VPU {

TaskGraphOrchestrator::TaskGraphOrchestrator(
    std::shared_ptr<HAL::KernelLibrary> kernel_lib,
    std::shared_ptr<HardwareProfile> hw_profile,
    int fusion_candidate_threshold)
: kernel_lib_(kernel_lib),
  hw_profile_(hw_profile),
  fusion_candidate_threshold_(fusion_candidate_threshold) {
    if (!kernel_lib_) {
        throw std::runtime_error("TaskGraphOrchestrator: KernelLibrary cannot be null.");
    }
    if (!hw_profile_) {
        throw std::runtime_error("TaskGraphOrchestrator: HardwareProfile cannot be null.");
    }
    std::cout << "[Pillar 6] TaskGraphOrchestrator initialized. Fusion threshold: "
              << fusion_candidate_threshold_ << ", Analysis interval: " << analysis_interval_ << " tasks."
              << std::endl;
}

void TaskGraphOrchestrator::record_executed_plan(const ExecutionPlan& plan) {
    plan_history_.push_back(plan);
    task_execution_counter_++;
    std::cout << "[Pillar 6] Recorded executed plan: " << plan.chosen_path_name
              << ". Total plans in history: " << plan_history_.size() << "." << std::endl;

    // Trigger analysis periodically
    if (task_execution_counter_ % analysis_interval_ == 0) {
        std::cout << "[Pillar 6] Task execution counter reached " << task_execution_counter_
                  << ". Triggering pattern analysis and fusion." << std::endl;
        analyze_and_fuse_patterns();
    }
}

void TaskGraphOrchestrator::analyze_and_fuse_patterns() {
    std::cout << "[Pillar 6] Analyzing plan history for fusion candidates..." << std::endl;
    if (plan_history_.empty()) {
        std::cout << "[Pillar 6] Plan history is empty. No patterns to analyze." << std::endl;
        return;
    }

    std::map<std::pair<std::string, std::string>, int> sequences = find_frequent_sequences();

    if (sequences.empty()) {
        std::cout << "[Pillar 6] No frequent sequences found for fusion." << std::endl;
        return;
    }

    for (const auto& pair_entry : sequences) {
        const auto& op_sequence = pair_entry.first;
        int count = pair_entry.second;
        std::cout << "[Pillar 6] Sequence <" << op_sequence.first << ", " << op_sequence.second
                  << "> appeared " << count << " times." << std::endl;
        if (count >= fusion_candidate_threshold_) {
            std::cout << "[Pillar 6] Sequence <" << op_sequence.first << ", " << op_sequence.second
                      << "> met fusion threshold (" << fusion_candidate_threshold_ << "). Attempting fusion." << std::endl;
            create_fused_kernel(op_sequence.first, op_sequence.second);
        }
    }
}

std::map<std::pair<std::string, std::string>, int> TaskGraphOrchestrator::find_frequent_sequences() {
    std::map<std::pair<std::string, std::string>, int> sequence_counts;
    if (plan_history_.size() < 2 && (plan_history_.empty() || plan_history_[0].steps.size() < 2)) { // Need enough history/steps for sequences
         std::cout << "[Pillar 6] Not enough plan history or steps within plans to find sequences." << std::endl;
        return sequence_counts;
    }

    for (const auto& plan : plan_history_) {
        if (plan.steps.size() < 2) {
            continue; // Need at least two steps for a sequence
        }
        for (size_t i = 0; i < plan.steps.size() - 1; ++i) {
            // We are interested in sequences of *operational* steps, not necessarily any step.
            // This check assumes that steps that have a base_operational_cost are the ones we'd fuse.
            // Or, more simply, just use the operation_name string if it's not a JIT/meta operation.
            const std::string& op1_name = plan.steps[i].operation_name;
            const std::string& op2_name = plan.steps[i+1].operation_name;

            // Avoid fusing JIT compilation steps or other meta-operations for now.
            // This is a simple heuristic. A more robust system might have flags on ExecutionStep.
            if (op1_name.find("JIT_") != std::string::npos || op2_name.find("JIT_") != std::string::npos ||
                op1_name.find("EXECUTE_") != std::string::npos || op2_name.find("EXECUTE_") != std::string::npos ) { // Avoid fusing JIT steps with their execution
                // Or more generally, avoid fusing things that are not in base_operational_costs
                 if (!hw_profile_->base_operational_costs.count(op1_name) || !hw_profile_->base_operational_costs.count(op2_name) ) {
                    continue;
                 }
            }

            // Also avoid fusing an operation with itself for this simple example
            if (op1_name == op2_name) continue;


            sequence_counts[{op1_name, op2_name}]++;
        }
    }
    return sequence_counts;
}

void TaskGraphOrchestrator::create_fused_kernel(const std::string& op1_name, const std::string& op2_name) {
    std::string new_kernel_name = "FUSED_" + op1_name + "_" + op2_name;

    // Check if kernel already exists (e.g. from a previous fusion)
    if (kernel_lib_->count(new_kernel_name)) {
        std::cout << "[Pillar 6] Fused kernel '" << new_kernel_name << "' already exists. Skipping creation." << std::endl;
        return;
    }

    // Conceptual: Add placeholder for the new fused kernel to KernelLibrary
    (*kernel_lib_)[new_kernel_name] = [new_kernel_name]() { // Capture new_kernel_name
        std::cout << "Executing FUSED KERNEL: " << new_kernel_name << std::endl;
        // Placeholder for actual fused operation logic
    };
    std::cout << "[Pillar 6] Conceptually added new fused kernel '" << new_kernel_name << "' to KernelLibrary." << std::endl;

    // Conceptual: Add an estimated cost for this new kernel to HardwareProfile
    double cost_op1 = hw_profile_->base_operational_costs.count(op1_name) ? hw_profile_->base_operational_costs.at(op1_name) : 100.0; // Default cost
    double cost_op2 = hw_profile_->base_operational_costs.count(op2_name) ? hw_profile_->base_operational_costs.at(op2_name) : 100.0; // Default cost
    double estimated_fused_cost = 0.8 * (cost_op1 + cost_op2); // Assume 20% efficiency gain from fusion

    hw_profile_->base_operational_costs[new_kernel_name] = estimated_fused_cost;
    std::cout << "[Pillar 6] Added estimated cost for '" << new_kernel_name << "' (" << estimated_fused_cost
              << ") to HardwareProfile base_operational_costs." << std::endl;

    // Potentially add a new sensitivity if the fused op has different characteristics
    // For example, if op1 was sensitive to X and op2 to Y, new_fused_op might be sensitive to both or neither.
    // hw_profile_->flux_sensitivities["lambda_FUSED_" + op1_name + "_" + op2_name] = some_estimated_sensitivity;
    // For now, we keep it simple and only add to base_operational_costs.
}

} // namespace VPU
