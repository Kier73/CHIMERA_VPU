#pragma once

#include "vpu_data_structures.h"
#include <map>
#include <string>

namespace VPU {

// Represents the hardware's known performance characteristics (the "beliefs").
// This is the core model that Pillar 5 will update.
struct HardwareProfile {
    // Defines the base cost for an operation, primarily predicting cycle_cost.
    // e.g., "CONV_DIRECT" -> 100.0 (arbitrary flux units, representing estimated cycles)
    // This cost is for the operation itself, assuming neutral or average data impact.
    std::map<std::string, double> base_operational_costs;

    // Defines the cost of changing data representation or setup costs.
    // e.g., "FFT_FORWARD" (if considered a transform), "JIT_COMPILE_SAXPY"
    // Key: string (e.g., "TRANSFORM_TIME_TO_FREQ")
    std::map<std::string, double> transform_costs;

    // Defines how sensitive an operation is to different data characteristics (flux per unit of metric).
    // These are the "lambdas" that Pillar 5 learns and updates.
    // Examples:
    // - "OPERATION_NAME_lambda_Sparsity" -> how cost changes with data sparsity.
    // - "OPERATION_NAME_lambda_AmplitudeFlux" -> how cost changes with amplitude flux.
    // - "OPERATION_NAME_lambda_hw_combined" -> new, for Hamming Weight sensitivity.
    std::map<std::string, double> flux_sensitivities;
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
