#include "dgm_archive.h"
#include <string> // Required for std::to_string in error messages

namespace DGM {

AgentArchive::AgentArchive() {
    // Constructor, if any specific initialization is needed
}

void AgentArchive::add_agent(const Agent& agent) {
    if (agents_.count(agent.agent_id)) {
        // Handle duplicate agent ID, e.g., throw error or log warning
        // For now, let's overwrite, but this might need a strategy
        // Or, better, ensure IDs are always unique before adding.
        // Consider: throw std::runtime_error("Attempted to add agent with duplicate ID: " + std::to_string(agent.agent_id));
    }
    agents_[agent.agent_id] = agent;
}

Agent& AgentArchive::get_agent(AgentIdType agent_id) {
    try {
        return agents_.at(agent_id);
    } catch (const std::out_of_range& oor) {
        // Consider a more specific exception type for DGM
        throw std::runtime_error("Agent with ID " + std::to_string(agent_id) + " not found in archive.");
    }
}

const Agent& AgentArchive::get_agent(AgentIdType agent_id) const {
    try {
        return agents_.at(agent_id);
    } catch (const std::out_of_range& oor) {
        throw std::runtime_error("Agent with ID " + std::to_string(agent_id) + " not found in archive (const).");
    }
}

bool AgentArchive::has_agent(AgentIdType agent_id) const {
    return agents_.count(agent_id) > 0;
}

std::vector<AgentIdType> AgentArchive::get_all_agent_ids() const {
    std::vector<AgentIdType> ids;
    ids.reserve(agents_.size());
    for (const auto& pair : agents_) {
        ids.push_back(pair.first);
    }
    return ids;
}

size_t AgentArchive::get_population_size() const {
    return agents_.size();
}

const std::map<AgentIdType, Agent>& AgentArchive::get_agents_map() const {
    return agents_;
}

} // namespace DGM
