#ifndef PILLAR2_CORTEX_H
#define PILLAR2_CORTEX_H

#include <vector>
#include <string>
#include <cmath>        // For std::sqrt, std::log2
#include <numeric>      // For std::accumulate (potentially)
#include <iostream>     // For std::cerr
#include <fftw3.h>      // Include actual FFTW3 header
#include "vpu_data_structures.h" // For EnrichedExecutionContext, DataProfile
#include "vpu.h"                 // For VPU_Task (Corrected from "api/vpu.h")
#include <memory>                // For std::make_shared
#include "IoTClient.h"           // For IoTClient integration

namespace VPU { // Changed namespace to VPU

    // Data structure to hold the results of the omnimorphic profiling (internal to Cortex)
    struct OmniProfile {
        double amplitude_flux = 0.0;
        double frequency_flux = 0.0;
        double entropy_flux = 0.0;
        double temporal_coherence = 0.0;
        // Add other relevant metrics as needed
    };

    class Cortex { // Renamed class to Cortex
    public:
        Cortex();
        ~Cortex();

        // Analyzes the data from VPU_Task and returns EnrichedExecutionContext
        EnrichedExecutionContext analyze(const VPU_Task& task);

        // Test helper to override IoT data for the next analyze call
        void set_next_iot_profile_override(const DataProfile& override_profile);

    private:
        // Profiles the omnimorphic characteristics of a given data stream
        OmniProfile profileOmni(const double* data, int num_elements);

        // Calculates Hamming Weight and Sparsity for a given data buffer
        static void calculate_hamming_weight_for_profile(const void* data, size_t num_bytes, DataProfile& profile);

        // IoT Client for fetching external sensor data
        std::unique_ptr<IoTClient> iot_client_;

        // Member to store the override profile
        std::unique_ptr<DataProfile> next_iot_override_;
        // Helper methods, if any, can be declared here.
    };

} // namespace VPU

#endif // PILLAR2_CORTEX_H
