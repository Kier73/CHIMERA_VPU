#pragma once

#include "dgm_agent.h"
#include "dgm_archive.h"
#include "dgm_selection.h"
#include "dgm_evolution.h" // For DGMEvolution::self_modify and DGMEvolution::evaluate
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

namespace DGM {

class DGMController {
public:
    DGMController(const std::string& initial_agent_source_placeholder,
                  int max_iterations,
                  int num_children_per_iteration = 1, // Default to 1 child for simplicity
                  const std::string& benchmark_placeholder = "default_benchmark");

    void run_evolutionary_loop();

    // Helper to get archive for testing/inspection
    const AgentArchive& get_archive() const;

private:
    AgentArchive archive_;
    std::unique_ptr<ParentSelector> parent_selector_; // Use unique_ptr as it depends on archive_

    int current_iteration_;
    int max_iterations_;
    int num_children_per_iteration_;
    AgentIdType next_agent_id_;
    std::string benchmark_placeholder_;

    AgentIdType generate_new_agent_id();
    bool is_agent_valid(const Agent& agent) const; // Placeholder
};

} // namespace DGM
