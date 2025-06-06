#pragma once

#include <string>
#include <vector>
#include <optional> // For optional parent_id

namespace DGM {

// Define a type for Agent IDs for clarity and future flexibility
using AgentIdType = int; // Or std::string for UUIDs later

// Constants for the DGM's mathematical model (from user feedback)
const double LAMBDA_PARAM = 10.0;
const double ALPHA_NAUGHT_PARAM = 0.5; // Performance midpoint

struct Agent {
    AgentIdType agent_id;
    std::optional<AgentIdType> parent_id; // Parent's ID, std::nullopt for initial agent
    std::string source_code_representation; // Placeholder for actual source code or its identifier
    double performance_score;
    std::string evaluation_log; // To store benchmark results or LLM feedback analysis
    int children_count;
    int creation_iteration;     // Iteration number when this agent was created

    // Constructor for easier initialization
    Agent(AgentIdType id,
          std::optional<AgentIdType> p_id,
          std::string source_repr,
          int iter,
          double perf_score = 0.0,
          std::string eval_log = "",
          int children = 0)
        : agent_id(id),
          parent_id(p_id),
          source_code_representation(std::move(source_repr)),
          performance_score(perf_score),
          evaluation_log(std::move(eval_log)),
          children_count(children),
          creation_iteration(iter) {}

    // Default constructor for map compatibility if needed, though direct init is better
    Agent() : agent_id(-1), parent_id(std::nullopt), performance_score(0.0), children_count(0), creation_iteration(-1) {}
};

} // namespace DGM
