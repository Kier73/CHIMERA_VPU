#pragma once

#include "vpu_data_structures.h"
#include "core/Pillar3_Orchestrator.h" // For HardwareProfile

namespace VPU {

// The FeedbackLoop completes the cognitive cycle by updating the VPU's beliefs.
class FeedbackLoop {
public:
    FeedbackLoop(std::shared_ptr<HardwareProfile> hw_profile, double quark_threshold = 0.15, double learning_rate = 0.1, double learning_rate_base_cost = 0.05);

    void learn_from_feedback(const LearningContext& context, double predicted_flux, const ActualPerformanceRecord& record);

private:
    // void apply_learning(double& belief, double observed, double predicted); // This was unused, can be removed or kept if planned for later

    std::shared_ptr<HardwareProfile> hw_profile_;
    const double QUARK_THRESHOLD; // e.g., 15% deviation
    const double LEARNING_RATE;
    const double LEARNING_RATE_BASE_COST; // Added member
};

} // namespace VPU
