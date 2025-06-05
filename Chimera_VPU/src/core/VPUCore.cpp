#include "core/VPUCore.h" // Self header
#include "pillars/Cortex.h"
#include "pillars/Orchestrator.h"
#include "pillars/Cerebellum.h"
#include "pillars/Feedback.h"
// No need to include Synapse.h if it's just VPU api
// No need to include IKernel.h directly if not used by VPUCore directly
// No need to include CPUDenseKernels.cpp (it's a source file)

VPUCore::VPUCore() :
    cortex_(std::make_unique<Cortex>()),
    orchestrator_(std::make_unique<Orchestrator>()),
    cerebellum_(std::make_unique<Cerebellum>()),
    feedback_(std::make_unique<Feedback>()) {
    // Constructor body can be empty for now
}

VPUCore::~VPUCore() = default; // Definition for the destructor
