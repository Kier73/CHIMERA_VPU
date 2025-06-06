#pragma once

#include "dgm_agent.h"
#include "dgm_archive.h"
#include <vector>
#include <string>
#include <map>
#include <random> // For std::mt19937, std::discrete_distribution

namespace DGM {

class ParentSelector {
public:
    // Constructor takes a const reference to the AgentArchive
    explicit ParentSelector(const AgentArchive& archive);

    // Selects a specified number of parents based on performance and novelty
    std::vector<AgentIdType> select_parents(int num_parents_to_select);

private:
    const AgentArchive& archive_; // Reference to the agent population
    std::mt19937 random_generator_; // For probabilistic sampling

    // Helper struct to store calculated weights for selection
    struct AgentSelectionStats {
        AgentIdType id;
        double scaled_performance_s_i;
        double novelty_bonus_h_i;
        double unnormalized_weight_w_i;
        double normalized_probability_p_i;
    };

    // Calculates s_i, h_i, w_i for all agents
    std::vector<AgentSelectionStats> calculate_agent_weights() const;
};

} // namespace DGM
