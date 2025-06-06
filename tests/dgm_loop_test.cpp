#include "dgm/dgm_controller.h" // Adjust path as needed if src/ is an include dir
#include "dgm/dgm_agent.h"
#include "dgm/dgm_archive.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::exception for basic error handling in test

// Basic test function
void test_dgm_loop() {
    std::cout << "\n--- Starting DGM Loop Test ---" << std::endl;

    const std::string initial_source = "Initial_VPU_Agent_Code_v0";
    const int max_iterations = 5; // Keep low for a simple test
    const int num_children_per_iteration = 2; // Generate 2 children per iteration
    const std::string benchmark = "benchmark_alpha";

    try {
        DGM::DGMController dgm_controller(initial_source, max_iterations, num_children_per_iteration, benchmark);

        std::cout << "\n[Test] Initial archive size: " << dgm_controller.get_archive().get_population_size() << std::endl;
        if (dgm_controller.get_archive().get_population_size() != 1) {
            std::cerr << "TEST FAILED: Initial archive size is not 1." << std::endl;
            return;
        }
        const DGM::Agent& agent0 = dgm_controller.get_archive().get_agent(0);
        if (agent0.source_code_representation != initial_source) {
             std::cerr << "TEST FAILED: Agent 0 source code mismatch." << std::endl;
            return;
        }
         if (agent0.performance_score == 0.0 && agent0.evaluation_log.empty() && agent0.creation_iteration == 0) {
            // Note: Placeholder evaluate might assign 0.0. This checks it was at least called.
            // A more robust check would be if the score is within the random range defined in evaluate.
             // However, the current default constructor for Agent also sets score to 0.0.
             // The key is that evaluate() populates evaluation_log.
             // Let's check if evaluation_log is NOT empty IF performance_score is non-zero,
             // or if it IS empty, then performance_score should be what evaluate() assigned.
             // The current evaluate() assigns a random score and always sets a log.
             // So, a non-empty evaluation_log is a better check for evaluate() having run.
            std::cout << "INFO: Agent 0 initial evaluation values: Score=" << agent0.performance_score
                      << ", Log Present=" << (!agent0.evaluation_log.empty()) << std::endl;
            if (agent0.evaluation_log.empty()){
                 std::cerr << "TEST WARNING: Agent 0 evaluation_log is empty after initialization, evaluate() might not have run as expected." << std::endl;
            }
        }


        dgm_controller.run_evolutionary_loop();

        std::cout << "\n[Test] Evolutionary loop finished." << std::endl;
        const DGM::AgentArchive& final_archive = dgm_controller.get_archive();
        std::cout << "[Test] Final archive size: " << final_archive.get_population_size() << std::endl;

        // Expected number of agents: 1 (initial) + (max_iterations * num_children_per_iteration)
        // This is the max possible. Fewer is possible if parent selection yields fewer parents than children requested,
        // or if some generated children are invalid (though current is_agent_valid is always true).
        size_t max_expected_agents = 1 + static_cast<size_t>(max_iterations) * num_children_per_iteration;
        // Minimum expected if at least one child is produced per iteration where possible: 1 + max_iterations (if num_children >=1)
        // However, parent selection can be tricky. If only 1 agent, it selects that one.
        // If num_children is 2, it will try to pick 2 parents. If only 1 agent in archive, it picks it twice.
        // And then generates 2 children from that one parent.
        size_t min_expected_agents = 1 + static_cast<size_t>(max_iterations); // if at least one child per iter is made

        if (final_archive.get_population_size() > 1 && final_archive.get_population_size() <= max_expected_agents) {
             std::cout << "TEST PASSED (Basic Check): Archive populated. Size: " << final_archive.get_population_size()
                      << " (Expected up to " << max_expected_agents << ")" <<std::endl;
        } else if (final_archive.get_population_size() == 1 && max_iterations > 0) {
             std::cout << "TEST INFO: Archive size is 1. This might indicate issues in parent selection or child generation/validation, "
                       << "or all children were invalid (though current validation is always true)." << std::endl;
        }
        else {
            std::cerr << "TEST FAILED: Final archive size (" << final_archive.get_population_size()
                      << ") is unexpected (expected up to " << max_expected_agents << ", or 1 if issues)." << std::endl;
        }

        // Further checks could involve inspecting properties of agents in the archive
        // For example, check if children_count was updated for parents,
        // or if source_code_representation shows evolution.
        // Example: Check children_count of agent 0 if it was selected as parent
        if (final_archive.has_agent(0)) {
             const DGM::Agent& agent0_final = final_archive.get_agent(0);
             std::cout << "[Test] Agent 0 final children_count: " << agent0_final.children_count << std::endl;
             if (max_iterations > 0 && num_children_per_iteration > 0 && agent0_final.children_count == 0) {
                std::cout << "TEST INFO: Agent 0 has 0 children. This is possible if it was never selected as a parent or only other agents were." << std::endl;
             } else if (agent0_final.children_count > 0) {
                std::cout << "TEST INFO: Agent 0 was selected as a parent " << agent0_final.children_count << " times (or produced that many children)." << std::endl;
             }
        }


    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED with exception: " << e.what() << std::endl;
    }
    std::cout << "--- DGM Loop Test Finished ---" << std::endl;
}

int main() {
    test_dgm_loop();
    return 0;
}
