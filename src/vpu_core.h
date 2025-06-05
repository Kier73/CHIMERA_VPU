#pragma once

#include "vpu.h" // For VPU_Task (Corrected from "api/vpu.h")
#include "core/Pillar2_Cortex.h"
#include "core/Pillar3_Orchestrator.h"
#include "core/Pillar4_Cerebellum.h"
#include "core/Pillar5_Feedback.h"
#include "hal/hal.h"
#include <memory> // Required for std::shared_ptr, std::unique_ptr, std::make_shared

// Corrected namespace for Cortex to avoid ambiguity if Chimera_VPU::Pillar2_Cortex::Pillar2_Cortex is a class
// Assuming Cortex here refers to the Pillar2_Cortex class.
// If Pillar2_Cortex is a namespace and contains a class named Cortex, this needs adjustment.
// For now, let's assume Pillar2_Cortex is the class name within Chimera_VPU namespace.
// This might require a using declaration or alias if there's a naming conflict or specific structure.
// For the provided structure, Chimera_VPU::Pillar2_Cortex::RepresentationalFluxAnalyzer is the actual class.
// Let's adjust to use the correct class name from Pillar2_Cortex.h
// "core/Pillar2_Cortex.h" should now define VPU::Cortex directly.
// No need for other forward declarations relating to Pillar2 types.
// #include "core/Pillar2_Cortex.h" // This is already present and correct.

namespace VPU {

// This central class acts as the "brainstem," coordinating the specialized
// functions of the different pillars (the VPU's "cortical regions").
class VPUCore {
public:
    VPUCore();
    void execute_task(VPU_Task& task);
    void print_current_beliefs();

private:
    void initialize_beliefs();
    void initialize_hal();

    // The shared model of the world
    std::shared_ptr<HardwareProfile> hw_profile_;

    // The HAL provides the tools
    std::shared_ptr<HAL::KernelLibrary> kernel_lib_;

    // The five pillars of the VPU cognitive cycle
    std::unique_ptr<VPU::Cortex> pillar2_cortex_; // Corrected type
    std::unique_ptr<Orchestrator> pillar3_orchestrator_;
    std::unique_ptr<Cerebellum> pillar4_cerebellum_;
    std::unique_ptr<FeedbackLoop> pillar5_feedback_;
};

} // namespace VPU
