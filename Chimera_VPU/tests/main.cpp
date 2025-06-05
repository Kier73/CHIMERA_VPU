#include <iostream>
#include "core/VPUCore.h" // To ensure VPUCore and its dependencies can be included
#include "vpu.h"       // To ensure VPU API can be included

int main() {
    std::cout << "Chimera VPU Test Main!" << std::endl;

    // Test instantiation of VPUCore
    VPUCore core;
    // Test instantiation of VPU (API)
    VPU api;

    // Add a simple message to indicate successful execution
    std::cout << "VPUCore and VPU objects instantiated successfully." << std::endl;

    return 0;
}
