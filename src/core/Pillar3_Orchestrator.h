#pragma once

#include "vpu_data_structures.h"
#include <map>
#include <string>

namespace VPU {

// Represents the hardware's known performance characteristics (the "beliefs").
// This is the core model that Pillar 5 will update.
struct HardwareProfile {
    // Defines the base cost (on 'silent' data) for an operation.
    // Key: string (e.g., "CONVOLUTION_DIRECT")
    std::map<std::string, double> base_operational_costs;

    // Defines the cost of changing data representation.
    // Key: string (e.g., "TRANSFORM_TIME_TO_FREQ")
    std::map<std::string, double> transform_costs;

    // Defines how sensitive an operation is to different flux metrics.
    // These are the "lambdas" that Pillar 5 learns and updates.
    std::map<std::string, double> flux_sensitivities; // e.g., "lambda_A", "lambda_sparsity"
};

class Orchestrator {
public:
    explicit Orchestrator(std::shared_ptr<HardwareProfile> hw_profile);
    std::vector<ExecutionPlan> determine_optimal_path(const EnrichedExecutionContext& context); // Changed return type

    // Method to enable/disable LLM usage
    void set_llm_path_generation(bool enable);

private:
    std::vector<ExecutionPlan> generate_candidate_paths(const std::string& task_type);
    double simulate_flux_cost(const ExecutionPlan& plan, const DataProfile& profile);

    // (Conceptual) Method to generate paths with LLM
    std::vector<ExecutionPlan> generate_paths_with_llm(const EnrichedExecutionContext& context);

    std::shared_ptr<HardwareProfile> hw_profile_;
    // Member variable to control LLM usage
    bool use_llm_for_paths_;
};

} // namespace VPU
