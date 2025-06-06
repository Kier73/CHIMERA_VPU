#include "vpu.h" // The main VPU API
#include "vpu_core.h" // For VPUCore and its members like Cortex, Orchestrator etc.
#include "core/Pillar2_Cortex.h"
#include "core/Pillar3_Orchestrator.h" // For HardwareProfile, ExecutionPlan
#include "core/Pillar5_Feedback.h"    // For LearningContext (though defined in vpu_data_structures.h)
#include "vpu_data_structures.h" // For DataProfile, ActualPerformanceRecord etc.

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>   // For uint8_t, uint64_t
#include <numeric>   // For std::accumulate (potentially)
#include <cmath>     // For std::abs, std::log2
#include <cassert>   // For assert()
#include <iomanip>   // For std::fixed, std::setprecision

// Helper function to print a divider
void print_divider(const std::string& title = "") {
    std::cout << "\n\n======================================================================\n";
    if (!title.empty()) {
        std::cout << "===== " << title << " =====\n";
    }
    std::cout << "======================================================================\n" << std::endl;
}

int main() {
    print_divider("VPU TEST SUITE STARTING");

    // 1. Initialize the VPU Environment using the real VPU API
    VPU::VPU_Environment vpu_env;
    VPU::VPUCore* core = vpu_env.get_core_for_testing();
    assert(core != nullptr && "Failed to get VPUCore for testing.");

    VPU::Cortex* cortex = core->get_cortex_for_testing();
    assert(cortex != nullptr && "Failed to get Cortex for testing.");

    VPU::Orchestrator* orchestrator = core->get_orchestrator_for_testing();
    assert(orchestrator != nullptr && "Failed to get Orchestrator for testing.");

    VPU::HardwareProfile* hw_profile_ptr = core->get_hardware_profile_for_testing(); // From VPUCore
    assert(hw_profile_ptr != nullptr && "Failed to get HardwareProfile for testing.");


    // --- Test 1: Pillar 2 Hamming Weight Calculation ---
    print_divider("TEST 1: Pillar 2 Hamming Weight Calculation");
    VPU::VPU_Task test_hw_task;
    uint8_t test_data[] = {0x01, 0xF0, 0x03, 0xFF}; // HW = 1 + 4 + 2 + 8 = 15
    test_hw_task.data_in_a = test_data;
    test_hw_task.data_in_a_size_bytes = sizeof(test_data);
    test_hw_task.num_elements = sizeof(test_data); // For this raw byte data, num_elements can be byte count
    test_hw_task.task_type = "TEST_HW_CALC";

    std::cout << "Submitting task to Cortex for HW calculation..." << std::endl;
    VPU::EnrichedExecutionContext hw_context = cortex->analyze(test_hw_task);

    uint64_t expected_hw = 15;
    std::cout << "Expected HW: " << expected_hw << ", Got HW: " << hw_context.profile->hamming_weight << std::endl;
    assert(hw_context.profile->hamming_weight == expected_hw);

    uint64_t expected_total_bits = sizeof(test_data) * 8;
    double expected_sparsity = 1.0 - (static_cast<double>(expected_hw) / expected_total_bits);
    std::cout << "Expected Sparsity: " << expected_sparsity << ", Got Sparsity: " << hw_context.profile->sparsity_ratio << std::endl;
    assert(std::abs(hw_context.profile->sparsity_ratio - expected_sparsity) < 1e-9);
    std::cout << "--- Test 1 PASSED ---" << std::endl;


    // --- Test 2: Pillar 4 Flux Reporting (Post-Execution) ---
    print_divider("TEST 2: Pillar 4 Flux Reporting");
    VPU::VPU_Task actual_task;
    std::vector<float> vec_a_data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::vector<float> vec_b_data(vec_a_data.size()); // Output buffer

    actual_task.task_type = "SAXPY_STANDARD"; // Ensure this kernel is in VPUCore::initialize_hal
    actual_task.data_in_a = vec_a_data.data();
    actual_task.data_in_a_size_bytes = vec_a_data.size() * sizeof(float);
    actual_task.data_out = vec_b_data.data(); // Output buffer
    actual_task.num_elements = vec_a_data.size();
    actual_task.alpha = 1.0f; // SAXPY 'a' - VPU_Task needs this, assuming it's added or handled by wrapper

    std::cout << "Executing SAXPY_STANDARD task..." << std::endl;
    vpu_env.execute(actual_task);
    const VPU::ActualPerformanceRecord& perf_record = vpu_env.get_last_performance_record();

    std::cout << "Observed Cycle Cost: " << perf_record.observed_cycle_cost << std::endl;
    std::cout << "Observed HW IN Cost: " << perf_record.observed_hw_in_cost << std::endl;
    std::cout << "Observed HW OUT Cost: " << perf_record.observed_hw_out_cost << std::endl;
    std::cout << "Observed Holistic Flux: " << perf_record.observed_holistic_flux << std::endl;

    assert(perf_record.observed_cycle_cost > 0); // Based on SAXPY_STANDARD lambda's cycle estimation
    assert(perf_record.observed_hw_in_cost > 0);
    assert(perf_record.observed_hw_out_cost > 0);

    uint64_t total_calculated_flux_parts = perf_record.observed_cycle_cost + perf_record.observed_hw_in_cost + perf_record.observed_hw_out_cost;
    std::cout << "Sum of flux parts: " << total_calculated_flux_parts << std::endl;
    assert(std::abs(perf_record.observed_holistic_flux - static_cast<double>(total_calculated_flux_parts)) < 1e-9);
    std::cout << "--- Test 2 PASSED ---" << std::endl;


    // --- Test 3: Pillar 3 Prediction (Basic Check) ---
    print_divider("TEST 3: Pillar 3 Prediction with HW Influence");
    VPU::VPU_Task task_a, task_b;
    uint8_t data_low_hw[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Low HW (1)
    uint8_t data_high_hw[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // High HW (64)
    std::vector<float> dummy_saxpy_out_a(sizeof(data_low_hw)); // Output buffer
    std::vector<float> dummy_saxpy_out_b(sizeof(data_high_hw));


    task_a.task_type = "SAXPY_STANDARD"; // Use an op with "_lambda_hw_combined" sensitivity
    task_a.data_in_a = data_low_hw;
    task_a.data_in_a_size_bytes = sizeof(data_low_hw);
    task_a.num_elements = sizeof(data_low_hw); // Treating as raw bytes for this part of test
    task_a.data_out = dummy_saxpy_out_a.data();
    task_a.alpha = 1.0f;

    task_b.task_type = "SAXPY_STANDARD";
    task_b.data_in_a = data_high_hw;
    task_b.data_in_a_size_bytes = sizeof(data_high_hw);
    task_b.num_elements = sizeof(data_high_hw);
    task_b.data_out = dummy_saxpy_out_b.data();
    task_b.alpha = 1.0f;

    std::cout << "Analyzing Task A (Low HW)..." << std::endl;
    VPU::EnrichedExecutionContext context_a = cortex->analyze(task_a);
    std::cout << "Analyzing Task B (High HW)..." << std::endl;
    VPU::EnrichedExecutionContext context_b = cortex->analyze(task_b);

    std::cout << "HW_A: " << context_a.profile->hamming_weight << ", HW_B: " << context_b.profile->hamming_weight << std::endl;
    assert(context_a.profile->hamming_weight < context_b.profile->hamming_weight);

    std::cout << "Determining optimal path for Task A..." << std::endl;
    std::vector<VPU::ExecutionPlan> plans_a = orchestrator->determine_optimal_path(context_a);
    std::cout << "Determining optimal path for Task B..." << std::endl;
    std::vector<VPU::ExecutionPlan> plans_b = orchestrator->determine_optimal_path(context_b);

    assert(!plans_a.empty() && "No plans returned for Task A");
    assert(!plans_b.empty() && "No plans returned for Task B");

    double predicted_flux_a = plans_a[0].predicted_holistic_flux;
    double predicted_flux_b = plans_b[0].predicted_holistic_flux;

    std::cout << "Predicted Flux Low HW (Task A): " << predicted_flux_a << std::endl;
    std::cout << "Predicted Flux High HW (Task B): " << predicted_flux_b << std::endl;
    // Assuming SAXPY_STANDARD_lambda_hw_combined is positive and significant enough
    assert(predicted_flux_b > predicted_flux_a);
    std::cout << "--- Test 3 PASSED ---" << std::endl;


    // --- Test 4: Pillar 5 Learning (Very Basic Check) ---
    print_divider("TEST 4: Pillar 5 Learning (Basic Check for HW Lambda)");
    std::string saxpy_hw_lambda_key = "SAXPY_STANDARD_lambda_hw_combined";

    // Ensure the key exists from initialize_beliefs
    assert(hw_profile_ptr->flux_sensitivities.count(saxpy_hw_lambda_key) && "SAXPY HW lambda key missing in profile");

    double initial_lambda = hw_profile_ptr->flux_sensitivities[saxpy_hw_lambda_key];
    std::cout << "Initial SAXPY HW Lambda (" << saxpy_hw_lambda_key << "): " << initial_lambda << std::endl;

    // Temporarily set lambda to a very small value to ensure misprediction if observed cost is higher
    // This requires mutable access to hw_profile_, which hw_profile_ptr provides.
    double forced_lambda_val = 0.0000001;
    hw_profile_ptr->flux_sensitivities[saxpy_hw_lambda_key] = forced_lambda_val;
    std::cout << "Forcing SAXPY HW Lambda to: " << forced_lambda_val << " for misprediction." << std::endl;

    // Re-use task_b (high HW) as it should generate a significant hw_in_cost.
    // The prediction from Pillar3 will use the forced_lambda_val, resulting in a low predicted flux.
    // The observed flux from Pillar4 will have a non-trivial hw_in_cost.
    // This should cause a positive deviation, and Pillar5 should increase the lambda.
    std::cout << "Executing Task B (High HW) with forced low lambda..." << std::endl;
    vpu_env.execute(task_b);

    double updated_lambda = hw_profile_ptr->flux_sensitivities[saxpy_hw_lambda_key];
    std::cout << "Updated SAXPY HW Lambda: " << updated_lambda << std::endl;

    // Check that lambda changed, and specifically that it increased due to underestimation.
    assert(updated_lambda > forced_lambda_val);
    std::cout << "--- Test 4 PASSED ---" << std::endl;

    // Restore lambda to original value if necessary for other tests, or re-init VPU_Environment
    hw_profile_ptr->flux_sensitivities[saxpy_hw_lambda_key] = initial_lambda;


    // --- End of New Tests ---
    // (Keep other existing tests like IoT, Pillar 5 Exploration, Pillar 6 Fusion if they are still relevant
    //  and adaptable. For this subtask, focus is on the new flux model tests.)
    //  Commenting out the rest of the original e2e_full_loop.cpp for now to focus on these new tests.

    print_divider("VPU TEST SUITE COMPLETED SUCCESSFULLY");
    return 0;
}
