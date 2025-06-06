#pragma once

#include "vpu_data_structures.h"
#include "core/Pillar3_Orchestrator.h" // For HardwareProfile
#include <random> // For std::mt19937 and std::uniform_real_distribution

namespace VPU {

// The FeedbackLoop completes the cognitive cycle by updating the VPU's beliefs.
class FeedbackLoop {
public:
    FeedbackLoop(std::shared_ptr<HardwareProfile> hw_profile,
                 double quark_threshold = 0.15,
                 double learning_rate = 0.1,
                 double learning_rate_base_cost = 0.05, // Added to match .cpp
                 double exploration_rate = 0.1);     // New parameter for exploration

    void learn_from_feedback(const LearningContext& context, double predicted_flux, const ActualPerformanceRecord& record);

    // Determines if the VPU should choose a suboptimal path for exploration
    bool should_explore();

    // Test helper to force exploration rate for deterministic testing
    void force_exploration_rate_for_testing(double rate);

private:
    void apply_learning(double& belief, double observed, double predicted); // This seems unused, consider removing or implementing

    std::shared_ptr<HardwareProfile> hw_profile_;
    const double QUARK_THRESHOLD; // e.g., 15% deviation
    const double LEARNING_RATE;
    const double LEARNING_RATE_BASE_COST; // Added to match .cpp

    // Exploration members
    double exploration_rate_;
    std::mt19937 random_generator_;
    std::uniform_real_distribution<double> distribution_;
};

} // namespace VPU
