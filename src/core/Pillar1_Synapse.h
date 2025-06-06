#pragma once

#include "api/vpu.h" // For VPU_Task definition
#include <memory>    // For std::shared_ptr or std::unique_ptr if needed later

// Forward declarations if other pillars are needed for detailed processing
namespace VPU {
namespace Pillar2 { class Pillar2_Cortex; } // Example if Pillar1 sends tasks to Pillar2

class Pillar1_Synapse {
public:
    // Constructor: May take a reference to the next pillar in the pipeline (e.g., Pillar2_Cortex)
    // For now, a simple constructor. Dependencies can be added later.
    Pillar1_Synapse();
    // explicit Pillar1_Synapse(std::shared_ptr<Pillar2::Pillar2_Cortex> cortex);


    // Accepts a task, performs initial validation/processing,
    // and forwards it for deeper analysis and execution.
    // Returns a status or result code (e.g., 0 for success, error code otherwise).
    // The exact return type can be elaborated later. For now, void or bool.
    bool submit_task(const VPU_Task& task);

private:
    // Internal state or helper methods if any.
    // For example, a pointer to the next stage (Pillar2_Cortex)
    // std::shared_ptr<Pillar2::Pillar2_Cortex> next_pillar_cortex;

    // Helper for basic validation
    bool validate_task(const VPU_Task& task) const;
};

} // namespace VPU
