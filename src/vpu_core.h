#pragma once

#include "vpu.h"
#include "core/Pillar1_Synapse.h" // Added
#include "core/Pillar2_Cortex.h"
#include "core/Pillar3_Orchestrator.h"
#include "core/Pillar4_Cerebellum.h"
#include "core/Pillar5_Feedback.h"
#include "core/Pillar6_TaskGraphOrchestrator.h" // Added Pillar 6
#include "hal/hal.h"
#include <memory>

namespace VPU {

class VPUCore {
public:
    VPUCore();
    void execute_task(VPU_Task& task);
    void print_current_beliefs();

private:
    void initialize_beliefs();
    void initialize_hal();

public: // Adding public getters for testing purposes
    Cortex* get_cortex_for_testing() { return pillar2_cortex_.get(); } // Added for Pillar 2
    Orchestrator* get_orchestrator_for_testing() { return pillar3_orchestrator_.get(); }
    Cerebellum* get_cerebellum_for_testing() { return pillar4_cerebellum_.get(); }
    FeedbackLoop* get_feedback_loop_for_testing() { return pillar5_feedback_.get(); }
    TaskGraphOrchestrator* get_task_graph_orchestrator_for_testing() { return pillar6_task_graph_orchestrator_.get(); }
    HardwareProfile* get_hardware_profile_for_testing() { return hw_profile_.get(); }
    HAL::KernelLibrary* get_kernel_library_for_testing() { return kernel_lib_.get(); }

private: // Original private members resume here
    std::shared_ptr<HardwareProfile> hw_profile_;
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;

    // The five pillars of the VPU cognitive cycle
    std::unique_ptr<Pillar1_Synapse> pillar1_synapse_; // Added
    std::unique_ptr<VPU::Cortex> pillar2_cortex_;
    std::unique_ptr<Orchestrator> pillar3_orchestrator_;
    std::unique_ptr<Cerebellum> pillar4_cerebellum_;
    std::unique_ptr<FeedbackLoop> pillar5_feedback_;
    std::unique_ptr<TaskGraphOrchestrator> pillar6_task_graph_orchestrator_; // Added Pillar 6
};

} // namespace VPU
