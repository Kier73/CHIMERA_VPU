#pragma once

#include "dgm_agent.h" // Include the Agent definition
#include <string>
#include <vector>
#include <map>
#include <stdexcept> // For std::runtime_error

namespace DGM {

class AgentArchive {
public:
    AgentArchive();

    void add_agent(const Agent& agent);
    Agent& get_agent(AgentIdType agent_id);
    const Agent& get_agent(AgentIdType agent_id) const; // Const version
    std::vector<AgentIdType> get_all_agent_ids() const;
    bool has_agent(AgentIdType agent_id) const;
    size_t get_population_size() const;

    // Provide access to the underlying map for selection logic
    const std::map<AgentIdType, Agent>& get_agents_map() const;


private:
    std::map<AgentIdType, Agent> agents_;
    // May add more sophisticated storage or indexing later
};

} // namespace DGM
