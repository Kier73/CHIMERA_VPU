#include "core/Pillar5_Feedback.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept> // Required for std::runtime_error

namespace VPU {

FeedbackLoop::FeedbackLoop(std::shared_ptr<HardwareProfile> hw_profile, double quark_threshold, double learning_rate)
: hw_profile_(hw_profile), QUARK_THRESHOLD(quark_threshold), LEARNING_RATE(learning_rate)
{
    if (!hw_profile_) {
        throw std::runtime_error("FeedbackLoop's HardwareProfile cannot be null.");
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

    // --- Credit Assignment: Was the error in a transform or the operation itself? ---
    // Note: The original code assumes .at() will find the key.
    // Adding checks for key existence before trying to access.
    if (!context.transform_key.empty() && hw_profile_->transform_costs.count(context.transform_key)) {
        // The error was in a fixed transform cost (e.g., FFT). Update that belief.
        double& belief = hw_profile_->transform_costs.at(context.transform_key);
        double old_belief = belief;

        // This is tricky - we need to assign blame. A simple heuristic:
        // Assume the entire deviation came from this single cost.
        // New_Belief = Old_Belief + (Deviation_Amount)
        // Deviation_Amount = Old_Belief * deviation (if deviation is % of old_belief)
        // OR Deviation_Amount = observed - predicted (if we want to adjust by the delta)
        // The original code `belief = belief + (deviation * belief);` means `belief * (1 + deviation)`
        // If predicted = old_belief, then belief_new = old_belief * (1 + (observed - old_belief)/old_belief) = old_belief * (observed/old_belief) = observed
        // So, this effectively sets the belief to the observed cost IF this one transform was the SOLE contributor.
        belief = record.observed_holistic_flux; // More direct: If this transform is blamed, its cost should be what was observed for it.
                                                // This is a strong assumption if multiple transforms/ops are in the plan.
                                                // A more nuanced approach would distribute the error.
                                                // For simplicity of this example, we'll stick to the idea that if a transform_key is provided,
                                                // it's the sole source of error for this learning update.

        std::cout << "    -> Updating transform cost '" << context.transform_key << "': " << old_belief << " -> " << belief << std::endl;

    } else if (!context.operation_key.empty() && hw_profile_->flux_sensitivities.count(context.operation_key)) {
        // The error was likely in the dynamic flux calculation. Adjust the relevant lambda.
        double& lambda_belief = hw_profile_->flux_sensitivities.at(context.operation_key);
        double old_belief = lambda_belief;

        // Apply a simple reinforcement learning rule
        // If we were too optimistic (observed > predicted), increase the lambda to penalize more.
        // If we were too pessimistic (observed < predicted), decrease the lambda.
        lambda_belief *= (1.0 + (deviation * LEARNING_RATE)); // This was the original logic.
                                                            // deviation is (obs-pred)/pred.
                                                            // if obs > pred, deviation is positive, lambda increases.
                                                            // if obs < pred, deviation is negative, lambda decreases.

        std::cout << "    -> Updating sensitivity '" << context.operation_key << "': " << old_belief << " -> " << lambda_belief << std::endl;
    } else {
         std::cout << "    -> Warning: Could not assign credit for Flux Quark. No valid transform_key or operation_key provided in LearningContext, or key not found in HardwareProfile." << std::endl;
    }
}

} // namespace VPU
