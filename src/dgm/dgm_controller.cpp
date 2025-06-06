#include "dgm_controller.h"
#include <iostream>     // For std::cout logging
#include <stdexcept>    // For std::runtime_error
#include <algorithm>    // For std::min
#include <vector>       // Required for std::vector<AgentIdType>

namespace DGM {

DGMController::DGMController(
    const std::string& initial_agent_source_placeholder,
    int max_iterations,
    int num_children_per_iteration,
    const std::string& benchmark_placeholder)
    : current_iteration_(0),
      max_iterations_(max_iterations),
      num_children_per_iteration_(num_children_per_iteration),
      next_agent_id_(0), // Start agent IDs from 0
      benchmark_placeholder_(benchmark_placeholder) {

    if (max_iterations_ <= 0) {
        throw std::invalid_argument("Max iterations must be positive.");
    }
    if (num_children_per_iteration_ <= 0) {
        throw std::invalid_argument("Number of children per iteration must be positive.");
    }

    std::cout << "DGMController: Initializing with max_iterations=" << max_iterations_
              << ", children_per_iteration=" << num_children_per_iteration_ << std::endl;

    // Initialize with Agent 0
    Agent agent_0(
        generate_new_agent_id(), // id = 0
        std::nullopt,            // parent_id
        initial_agent_source_placeholder,
        current_iteration_       // creation_iteration
    );

    DGMEvolution::evaluate(agent_0, benchmark_placeholder_); // Evaluate initial agent
    archive_.add_agent(agent_0);
    std::cout << "DGMController: Initialized Agent 0. Performance: " << agent_0.performance_score
              << ", Source: " << agent_0.source_code_representation << std::endl;

    // Initialize ParentSelector after archive_ has at least one agent
    parent_selector_ = std::make_unique<ParentSelector>(archive_);
}

AgentIdType DGMController::generate_new_agent_id() {
    return next_agent_id_++;
}

bool DGMController::is_agent_valid(const Agent& agent) const {
    // Placeholder: In a real system, this would check if code compiles, runs without crashing, etc.
    // For now, all conceptually "modified" agents are considered valid.
    std::cout << "  DGMController: Validating Agent ID: " << agent.agent_id << " (Placeholder: always valid)" << std::endl;
    return true;
}

const AgentArchive& DGMController::get_archive() const {
    return archive_;
}

void DGMController::run_evolutionary_loop() {
    std::cout << "\nDGMController: Starting evolutionary loop..." << std::endl;

    for (current_iteration_ = 1; current_iteration_ <= max_iterations_; ++current_iteration_) {
        std::cout << "\n--- DGM Iteration " << current_iteration_ << " ---" << std::endl;

        if (archive_.get_population_size() == 0) {
            std::cerr << "DGMController: Archive is empty. Cannot select parents. Stopping." << std::endl;
            break;
        }

        // Determine how many parents to actually try to select
        // Cannot select more parents than available in the archive for meaningful selection variety,
        // but ParentSelector itself handles if num_parents_to_select > population_size by returning all.
        // The number of children we want to generate dictates how many parents we *need*.
        int parents_needed_for_children = num_children_per_iteration_;


        std::vector<AgentIdType> selected_parent_ids = parent_selector_->select_parents(parents_needed_for_children);

        if (selected_parent_ids.empty() && archive_.get_population_size() > 0) {
            std::cerr << "DGMController: No parents were selected by ParentSelector, but archive is not empty. "
                      << "This might happen if all agents have zero selection probability. "
                      << "Stopping to prevent infinite loop or unproductive iterations." << std::endl;
            // For robustness, one might implement a fallback here, like picking a random agent,
            // but the current ParentSelector logic tries to assign uniform probability if all weights are zero.
            // If it still returns empty, it implies an issue or an archive state that needs debugging.
            break;
        }

        int children_generated_this_iteration = 0;
        for (AgentIdType parent_id : selected_parent_ids) {
            // This loop will run parents_needed_for_children times if enough distinct parents were selected
            // and ParentSelector samples with replacement.
            // If num_children_per_iteration is 1, this loop runs once.
            if (children_generated_this_iteration >= num_children_per_iteration_) {
                 // This break ensures we don't generate more children than specified by num_children_per_iteration_
                 // even if select_parents returned more parent IDs (e.g. due to sampling with replacement for a small pop)
                break;
            }

            Agent& parent_agent = archive_.get_agent(parent_id); // Get non-const reference to update children_count

            Agent child_agent = DGMEvolution::self_modify(parent_agent, generate_new_agent_id(), current_iteration_);
            std::cout << "DGMController: Generated Child ID: " << child_agent.agent_id
                      << " from Parent ID: " << parent_agent.agent_id << std::endl;

            parent_agent.children_count++; // Increment after successful retrieval and before potential errors in child processing

            DGMEvolution::evaluate(child_agent, benchmark_placeholder_);

            if (is_agent_valid(child_agent)) {
                archive_.add_agent(child_agent);
                std::cout << "DGMController: Child ID: " << child_agent.agent_id
                          << " is valid. Score: " << child_agent.performance_score
                          << ". Added to Archive. Archive size: " << archive_.get_population_size() << std::endl;
            } else {
                std::cout << "DGMController: Child ID: " << child_agent.agent_id
                          << " is invalid. Discarding." << std::endl;
                // Note: parent_agent.children_count was already incremented. This is a philosophical point:
                // was a child "produced" even if invalid? Current logic says yes.
            }
            children_generated_this_iteration++;
        }
         if (children_generated_this_iteration == 0 && archive_.get_population_size() > 0) {
             std::cout << "DGMController: No children generated this iteration (selected_parent_ids might have been empty or processed fully)." << std::endl;
         }
    }
    std::cout << "\nDGMController: Evolutionary loop completed after " << (current_iteration_ > max_iterations_ ? max_iterations_ : current_iteration_ -1)
              << " iterations." << std::endl; // Adjust final iteration count display
}

} // namespace DGM
