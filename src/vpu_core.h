#pragma once

#include "vpu.h"
#include "core/Pillar1_Synapse.h" // Added
#include "core/Pillar2_Cortex.h"
#include "core/Pillar3_Orchestrator.h"
#include "core/Pillar4_Cerebellum.h"
#include "core/Pillar5_Feedback.h"
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

    std::shared_ptr<HardwareProfile> hw_profile_;
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;

    // The five pillars of the VPU cognitive cycle
    std::unique_ptr<Pillar1_Synapse> pillar1_synapse_; // Added
    std::unique_ptr<VPU::Cortex> pillar2_cortex_;
    std::unique_ptr<Orchestrator> pillar3_orchestrator_;
    std::unique_ptr<Cerebellum> pillar4_cerebellum_;
    std::unique_ptr<FeedbackLoop> pillar5_feedback_;
};

} // namespace VPU
