#ifndef CORE_VPUCORE_H
#define CORE_VPUCORE_H

#include <memory>

// Forward declarations for pillar classes
class Cortex;
class Orchestrator;
class Cerebellum;
class Feedback;
// Synapse is the public API (vpu.h), so not forward declared here unless it's also a pillar.
// Based on the description, Synapse is the public API and task interceptor,
// so it's likely represented by 'VPU' in vpu.h or a similar top-level class.
// VPUCore then manages the internal pillars.

class VPUCore {
public:
    VPUCore();
    ~VPUCore();

private:
    std::unique_ptr<Cortex> cortex_;
    std::unique_ptr<Orchestrator> orchestrator_;
    std::unique_ptr<Cerebellum> cerebellum_;
    std::unique_ptr<Feedback> feedback_;
};

#endif // CORE_VPUCORE_H
