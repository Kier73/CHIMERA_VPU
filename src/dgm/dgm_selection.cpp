#include "dgm_selection.h"
#include <cmath>     // For std::exp
#include <numeric>   // For std::accumulate
#include <algorithm> // For std::transform, std::sort (if needed for deterministic selection first)
#include <iostream>  // For debugging/logging (optional)
#include <vector>

namespace DGM {

ParentSelector::ParentSelector(const AgentArchive& archive)
    : archive_(archive), random_generator_(std::random_device{}()) {
}

std::vector<ParentSelector::AgentSelectionStats> ParentSelector::calculate_agent_weights() const {
    std::vector<AgentSelectionStats> stats_list;
    const auto& agents = archive_.get_agents_map();
    if (agents.empty()) {
        return stats_list;
    }

    stats_list.reserve(agents.size());
    double total_unnormalized_weight = 0.0;

    for (const auto& pair : agents) {
        const Agent& agent = pair.second;
        AgentSelectionStats stats;
        stats.id = agent.agent_id;

        // Step 1: Sigmoid-Scaled Performance (s_i)
        stats.scaled_performance_s_i = 1.0 / (1.0 + std::exp(-LAMBDA_PARAM * (agent.performance_score - ALPHA_NAUGHT_PARAM)));

        // Step 2: Novelty Bonus (h_i)
        // Ensure children_count is handled correctly if it can be negative or zero, though problem implies non-negative.
        // The formula 1.0 / (1.0 + children_count) is fine for non-negative counts.
        stats.novelty_bonus_h_i = 1.0 / (1.0 + static_cast<double>(agent.children_count));

        // Step 3: Unnormalized Weight (w_i)
        stats.unnormalized_weight_w_i = stats.scaled_performance_s_i * stats.novelty_bonus_h_i;

        stats_list.push_back(stats);
        total_unnormalized_weight += stats.unnormalized_weight_w_i;
    }

    // Step 4: Selection Probability (p_i) - Normalize weights
    if (total_unnormalized_weight > 1e-9) { // Use a small epsilon to avoid division by zero if all weights are very close to 0
        for (auto& stats : stats_list) {
            stats.normalized_probability_p_i = stats.unnormalized_weight_w_i / total_unnormalized_weight;
        }
    } else if (!stats_list.empty()) { // All weights are effectively zero (or negative, which shouldn't happen with current formulas)
         double uniform_prob = 1.0 / stats_list.size();
         for (auto& stats : stats_list) {
            stats.normalized_probability_p_i = uniform_prob;
        }
    }

    return stats_list;
}

std::vector<AgentIdType> ParentSelector::select_parents(int num_parents_to_select) {
    std::vector<AgentIdType> selected_parent_ids;
    const auto& agents_map = archive_.get_agents_map();

    if (agents_map.empty() || num_parents_to_select <= 0) {
        return selected_parent_ids; // No parents to select or none requested
    }

    std::vector<AgentSelectionStats> agent_stats = calculate_agent_weights();
    if (agent_stats.empty()) {
        // This can happen if agents_map was not empty but calculate_agent_weights returned empty (e.g. error state)
        // Or if all agents had such values that weights became NaN or inf, though current formulas are robust.
        return selected_parent_ids;
    }

    // Prepare for discrete distribution sampling
    std::vector<double> probabilities;
    std::vector<AgentIdType> agent_ids_for_sampling;

    probabilities.reserve(agent_stats.size());
    agent_ids_for_sampling.reserve(agent_stats.size());

    for(const auto& stats : agent_stats) {
        // Ensure probabilities are non-negative for std::discrete_distribution
        probabilities.push_back(std::max(0.0, stats.normalized_probability_p_i));
        agent_ids_for_sampling.push_back(stats.id);
    }

    // If the number of available agents is less than or equal to num_parents_to_select,
    // return all available agents. This avoids issues with discrete_distribution
    // if sum of probabilities is zero or if trying to sample more unique items than available.
    // Note: This means if num_parents_to_select = N and N agents exist, all N are returned (no probabilistic selection).
    if (agents_map.size() <= static_cast<size_t>(num_parents_to_select)) {
        selected_parent_ids.reserve(agent_ids_for_sampling.size());
        for(const auto& id : agent_ids_for_sampling) {
            selected_parent_ids.push_back(id);
        }
        // If unique parents are strictly needed and num_parents_to_select was higher than available,
        // this correctly returns only the available unique parents.
        return selected_parent_ids;
    }

    // Check if sum of probabilities is valid for discrete_distribution
    double sum_of_probs = std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
    if (sum_of_probs < 1e-9) { // If all probabilities are effectively zero
        // Fallback: Select randomly or return empty, or select based on some other criteria.
        // For now, let's select randomly from the available agent IDs without weights.
        // This case should be rare if uniform probability is assigned when total_unnormalized_weight is zero.
        std::uniform_int_distribution<size_t> uniform_dist(0, agent_ids_for_sampling.size() - 1);
        selected_parent_ids.reserve(num_parents_to_select);
        for (int i = 0; i < num_parents_to_select; ++i) {
             if (agent_ids_for_sampling.empty()) break;
             selected_parent_ids.push_back(agent_ids_for_sampling[uniform_dist(random_generator_)]);
        }
        return selected_parent_ids;
    }


    std::discrete_distribution<int> distribution(probabilities.begin(), probabilities.end());

    selected_parent_ids.reserve(num_parents_to_select);
    for (int i = 0; i < num_parents_to_select; ++i) {
        // This loop assumes sampling with replacement. If unique parents are strictly needed,
        // and num_parents_to_select < agents_map.size(), a different strategy is needed
        // (e.g., std::sample, or draw one-by-one and remove/mark as selected).
        // The problem statement implies standard probabilistic selection, discrete_distribution handles this.
        int sampled_idx = distribution(random_generator_);
        selected_parent_ids.push_back(agent_ids_for_sampling[sampled_idx]);
    }

    // Optional logging for verification
    // std::cout << "Selected Parent IDs: ";
    // for (AgentIdType id : selected_parent_ids) {
    //     std::cout << id << " ";
    // }
    // std::cout << std::endl;

    return selected_parent_ids;
}

} // namespace DGM
