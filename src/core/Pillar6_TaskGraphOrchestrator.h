#pragma once

#include "vpu_data_structures.h"
#include "hal/hal.h" // For HAL::KernelLibrary
#include "core/Pillar3_Orchestrator.h" // For HardwareProfile (though defined in Pillar3_Orchestrator.h, it's a general structure)
#include <vector>
#include <string>
#include <map>
#include <memory> // For std::shared_ptr
#include <utility> // For std::pair

namespace VPU {

class TaskGraphOrchestrator {
public:
    TaskGraphOrchestrator(std::shared_ptr<HAL::KernelLibrary> kernel_lib,
                          std::shared_ptr<HardwareProfile> hw_profile,
                          int fusion_candidate_threshold = 10);

    void record_executed_plan(const ExecutionPlan& plan);
    void analyze_and_fuse_patterns();

    // Test helpers
    void set_fusion_candidate_threshold_for_testing(int threshold) { fusion_candidate_threshold_ = threshold; }
    void set_analysis_interval_for_testing(int interval) { analysis_interval_ = interval; }
    void reset_task_execution_counter_for_testing() { task_execution_counter_ = 0; }


private:
    // Finds frequent sequences of two operations
    std::map<std::pair<std::string, std::string>, int> find_frequent_sequences();

    // Conceptually creates and registers a new fused kernel
    void create_fused_kernel(const std::string& op1_name, const std::string& op2_name);

    std::vector<ExecutionPlan> plan_history_;
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;
    std::shared_ptr<HardwareProfile> hw_profile_;
    int fusion_candidate_threshold_;
    int task_execution_counter_ = 0; // To trigger analysis periodically
    int analysis_interval_ = 5;     // Analyze every 5 tasks, for example
};

} // namespace VPU
