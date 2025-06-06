#include "core/Pillar5_Feedback.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept> // Required for std::runtime_error
#include <random>    // For std::random_device for seeding

namespace VPU {

FeedbackLoop::FeedbackLoop(std::shared_ptr<HardwareProfile> hw_profile,
                           double quark_threshold,
                           double learning_rate,
                           double learning_rate_base_cost,
                           double exploration_rate) // Added exploration_rate
: hw_profile_(hw_profile),
  QUARK_THRESHOLD(quark_threshold),
  LEARNING_RATE(learning_rate),
  LEARNING_RATE_BASE_COST(learning_rate_base_cost),
  exploration_rate_(exploration_rate),
  distribution_(0.0, 1.0) // Initialize distribution
{
    if (!hw_profile_) {
        throw std::runtime_error("FeedbackLoop's HardwareProfile cannot be null.");
    }
    // Seed the random number generator
    random_generator_.seed(std::random_device{}());
    std::cout << "[Pillar 5] FeedbackLoop initialized with exploration rate: " << exploration_rate_ * 100 << "%." << std::endl;
    }
}

// This is the core learning function.
void FeedbackLoop::learn_from_feedback(const LearningContext& context, double predicted_flux, const ActualPerformanceRecord& record) {
    std::cout << "[Pillar 5] Hippocampus: Analyzing feedback..." << std::endl;
    std::cout << "  -> Predicted Flux: " << predicted_flux << ", Observed Flux: " << record.observed_holistic_flux << std::endl;

    if (predicted_flux == 0 && record.observed_holistic_flux == 0) { // Both zero, no deviation
        std::cout << "  ==> Result: Predicted and Observed flux are both zero. Beliefs are stable." << std::endl;
        return;
    }
    if (predicted_flux == 0 && record.observed_holistic_flux != 0) { // Predicted zero, but there was a cost
        std::cout << "  ==> Result: **FLUX QUARK DETECTED!** Predicted zero flux, but observed " << record.observed_holistic_flux << ". Updating beliefs." << std::endl;
        // This case requires special handling as deviation would be infinite.
        // We'll directly adjust the belief based on the observed cost.
        // This heuristic assumes the 'operation_key' is the one to blame if 'transform_key' is empty.
        if (!context.transform_key.empty() && hw_profile_->transform_costs.count(context.transform_key)) {
             double& belief = hw_profile_->transform_costs.at(context.transform_key);
             double old_belief = belief;
             belief = record.observed_holistic_flux; // Set to observed
             std::cout << "    -> Updating transform cost '" << context.transform_key << "' (predicted zero): " << old_belief << " -> " << belief << std::endl;
        } else if (!context.operation_key.empty() && hw_profile_->flux_sensitivities.count(context.operation_key)) {
            double& lambda_belief = hw_profile_->flux_sensitivities.at(context.operation_key);
            double old_belief = lambda_belief;
            // If lambda was zero or very small, and we got a non-zero cost, it needs a significant bump.
            // This is a simple heuristic; a more robust system might use a default starting value or a portion of the observed cost.
            lambda_belief = std::max(lambda_belief, 0.01) + (record.observed_holistic_flux * LEARNING_RATE);
            std::cout << "    -> Updating sensitivity '" << context.operation_key << "' (predicted zero): " << old_belief << " -> " << lambda_belief << std::endl;
        }
        return;
    }


    double deviation = (record.observed_holistic_flux - predicted_flux) / predicted_flux;

    if (std::abs(deviation) < QUARK_THRESHOLD) {
        std::cout << "  ==> Result: Deviation (" << std::fixed << std::setprecision(2) << deviation * 100 << "%) is within threshold. Beliefs are stable." << std::endl;
        return;
    }

    // --- FLUX QUARK DETECTED ---
    std::cout << "  ==> Result: **FLUX QUARK DETECTED!** Deviation is " << std::fixed << std::setprecision(2) << deviation * 100 << "%. Updating beliefs." << std::endl;

    bool belief_updated = false;
    // 1. Try to update transform cost first
    if (!context.transform_key.empty() && hw_profile_->transform_costs.count(context.transform_key)) {
        double& belief = hw_profile_->transform_costs.at(context.transform_key);
        double old_belief = belief;
        // Simple update: adjust by a portion of the deviation in flux
        // (observed_holistic_flux - predicted_flux) is the total error.
        // A simple heuristic: this transform is responsible for this amount of error.
        belief += (record.observed_holistic_flux - predicted_flux) * LEARNING_RATE;
        if (belief < 1.0) belief = 1.0; // Ensure cost doesn't go below a very small positive number
        std::cout << "    -> Updating transform cost '" << context.transform_key << "': " << old_belief << " -> " << belief << std::endl;
        belief_updated = true;
    }

    // 2. If not a transform error, or in addition, update base operational cost
    if (!context.main_operation_name.empty() && hw_profile_->base_operational_costs.count(context.main_operation_name)) {
        double& belief = hw_profile_->base_operational_costs.at(context.main_operation_name);
        double old_belief = belief;
        // Adjust base cost. The 'deviation' is overall percentage error.
        // Apply this percentage error (scaled by learning rate) to the current belief for this op.
        belief += belief * deviation * LEARNING_RATE_BASE_COST;
        if (belief < 1.0) belief = 1.0; // Ensure cost doesn't go too low
        std::cout << "    -> Updating base operational cost '" << context.main_operation_name << "': " << old_belief << " -> " << belief << std::endl;
        belief_updated = true;
    }

    // 3. Update flux sensitivity (lambda)
    if (!context.operation_key.empty() && hw_profile_->flux_sensitivities.count(context.operation_key)) {
        double& lambda_belief = hw_profile_->flux_sensitivities.at(context.operation_key);
        double old_belief = lambda_belief;
        // Apply a simple reinforcement learning rule based on overall deviation
        lambda_belief *= (1.0 + (deviation * LEARNING_RATE));
        if (lambda_belief < 0) lambda_belief = 0; // Sensitivity shouldn't be negative
        std::cout << "    -> Updating sensitivity '" << context.operation_key << "': " << old_belief << " -> " << lambda_belief << std::endl;
        belief_updated = true;
    }

    if (!belief_updated) {
        std::cout << "    -> No specific belief component (transform, base op cost, or sensitivity) could be targeted for update based on context." << std::endl;
    }
}

// Determines if the VPU should choose a suboptimal path for exploration
bool FeedbackLoop::should_explore() {
    double random_value = distribution_(random_generator_);
    bool explore = random_value < exploration_rate_;
    if (explore) {
        std::cout << "[Pillar 5] FeedbackLoop: Decision to EXPLORE (Random value " << random_value << " < Exploration rate " << exploration_rate_ << ")" << std::endl;
    }
    return explore;
}

// Placeholder for apply_learning, if it were to be used.
// void FeedbackLoop::apply_learning(double& belief, double observed, double predicted) {
//     // Implementation would go here
// }

void FeedbackLoop::force_exploration_rate_for_testing(double rate) {
    exploration_rate_ = rate;
    std::cout << "[Pillar 5] FeedbackLoop: Exploration rate FORCED to " << exploration_rate_ * 100 << "% for testing." << std::endl;
}


} // namespace VPU
