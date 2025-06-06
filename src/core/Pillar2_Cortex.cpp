#include "core/Pillar2_Cortex.h" // Corresponding header
#include <vector>
#include <cmath>        // For std::sqrt, std::log2, std::abs
#include <numeric>      // For std::accumulate (if needed elsewhere)
#include <iostream>     // For std::cerr
#include <algorithm>    // For std::fill (if needed elsewhere)
#include <cstdint>      // For uint8_t, uint64_t

// The actual fftw3.h is already included in Pillar2_Cortex.h
// No need for simulated FFTW functions or typedefs here.

#include "nlohmann/json.hpp" // For JSON parsing (used conceptually for IoT data)

namespace VPU { // Changed namespace

    Cortex::Cortex() { // Renamed class
        // Initialize IoTClient with placeholder values
        try {
            iot_client_ = std::make_unique<IoTClient>("localhost", 12345);
            std::cout << "[Pillar 2] Cortex: IoTClient initialized conceptually for localhost:12345." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Pillar 2] Cortex: Failed to initialize IoTClient: " << e.what() << std::endl;
            iot_client_ = nullptr; // Ensure it's null if construction fails
        }
        std::cout << "[Pillar 2] Cortex initialized." << std::endl;
    }

    Cortex::~Cortex() { // Renamed class
        // Destructor implementation (if any)
    }

    void Cortex::set_next_iot_profile_override(const DataProfile& override_profile) {
        next_iot_override_ = std::make_unique<DataProfile>(override_profile);
        std::cout << "[Pillar 2] Cortex: Test override for IoT data set for next analyze call." << std::endl;
    }

    // Public method to analyze task data
    EnrichedExecutionContext Cortex::analyze(const VPU_Task& task) {
        std::cout << "[Pillar 2] Cortex: Analyzing task '" << task.task_type << "'..." << std::endl;

        OmniProfile omni_profile;
        if (task.data_in_a && task.num_elements > 0) {
            // Assuming task.data_in_a is const double* and task.num_elements is int for profileOmni.
            // This cast is potentially unsafe if task.data_in_a is not actually pointing to doubles.
            // A real system would need robust type checking or a safer way to pass data.
            const double* data_ptr = static_cast<const double*>(task.data_in_a);
            omni_profile = profileOmni(data_ptr, static_cast<int>(task.num_elements));
        } else {
            std::cerr << "Warning: Cortex::analyze called with null data or zero elements for profiling." << std::endl;
            // omni_profile will be default (all zeros)
        }

        auto data_profile_ptr = std::make_shared<DataProfile>();
        data_profile_ptr->amplitude_flux = omni_profile.amplitude_flux;
        data_profile_ptr->frequency_flux = omni_profile.frequency_flux;
        data_profile_ptr->entropy_flux = omni_profile.entropy_flux;

        std::cout << "  -> OmniProfile generated: AF=" << data_profile_ptr->amplitude_flux
                  << ", FF=" << data_profile_ptr->frequency_flux
                  << ", EF=" << data_profile_ptr->entropy_flux << std::endl;

        // Calculate Hamming Weight profile
        if (task.data_in_a && task.data_in_a_size_bytes > 0) { // Use data_in_a_size_bytes
            const void* hw_data_ptr = task.data_in_a;
            size_t hw_num_bytes = task.data_in_a_size_bytes;
            Cortex::calculate_hamming_weight_for_profile(hw_data_ptr, hw_num_bytes, *data_profile_ptr);

            std::cout << "  -> HW Profile generated (from data_in_a): HW=" << data_profile_ptr->hamming_weight
                      << ", Sparsity=" << data_profile_ptr->sparsity_ratio << std::endl;
        } else {
            // Set default Hamming weight and sparsity (already done by DataProfile constructor, but explicit for clarity)
            data_profile_ptr->hamming_weight = 0;
            data_profile_ptr->sparsity_ratio = 1.0;
            std::cout << "  -> HW Profile not generated due to null data or zero elements (defaults set)." << std::endl;
        }

        // --- Populate DataProfile with IoT Sensor Data (Conceptual/Dummy) ---
        if (next_iot_override_) {
            std::cout << "  -> Using TEST OVERRIDE for IoT data." << std::endl;
            data_profile_ptr->power_draw_watts = next_iot_override_->power_draw_watts;
            data_profile_ptr->temperature_celsius = next_iot_override_->temperature_celsius;
            data_profile_ptr->network_latency_ms = next_iot_override_->network_latency_ms;
            data_profile_ptr->network_bandwidth_mbps = next_iot_override_->network_bandwidth_mbps;
            data_profile_ptr->io_throughput_mbps = next_iot_override_->io_throughput_mbps;
            data_profile_ptr->data_quality_score = next_iot_override_->data_quality_score;
            next_iot_override_.reset(); // Clear override after use
        } else if (iot_client_) {
            std::cout << "  -> Fetching IoT data (conceptually)..." << std::endl;
            try {
                // Conceptual calls (commented out to avoid runtime errors without actual IoT server)
                // nlohmann::json power_data = iot_client_->getDeviceStatus("power_sensor_001");
                // nlohmann::json thermal_data = iot_client_->getDeviceStatus("thermal_sensor_001");
                // nlohmann::json network_data = iot_client_->getDeviceStatus("network_monitor_001");
                // nlohmann::json storage_data = iot_client_->getDeviceStatus("storage_monitor_001");
                // nlohmann::json quality_data = iot_client_->getDeviceStatus("data_quality_sensor_001");

                // Using dummy data for now
                data_profile_ptr->power_draw_watts = 75.5; // Example value
                // if (!power_data.empty() && power_data.contains("current_watts")) {
                //     data_profile_ptr->power_draw_watts = power_data["current_watts"].get<double>();
                // }

                data_profile_ptr->temperature_celsius = 65.2; // Example value
                // if (!thermal_data.empty() && thermal_data.contains("current_temp_c")) {
                //     data_profile_ptr->temperature_celsius = thermal_data["current_temp_c"].get<double>();
                // }

                data_profile_ptr->network_latency_ms = 15.3; // Example value
                // if (!network_data.empty() && network_data.contains("latency_ms")) {
                //     data_profile_ptr->network_latency_ms = network_data["latency_ms"].get<double>();
                // }

                data_profile_ptr->network_bandwidth_mbps = 980.0; // Example value
                // if (!network_data.empty() && network_data.contains("bandwidth_mbps")) {
                //     data_profile_ptr->network_bandwidth_mbps = network_data["bandwidth_mbps"].get<double>();
                // }

                data_profile_ptr->io_throughput_mbps = 250.0; // Example value
                // if(!storage_data.empty() && storage_data.contains("throughput_mbps")) {
                //    data_profile_ptr->io_throughput_mbps = storage_data["throughput_mbps"].get<double>();
                // }

                data_profile_ptr->data_quality_score = 0.95; // Example value
                // if(!quality_data.empty() && quality_data.contains("score")) {
                //    data_profile_ptr->data_quality_score = quality_data["score"].get<double>();
                // }

                std::cout << "  -> IoT Data (Dummy): Power=" << data_profile_ptr->power_draw_watts << "W"
                          << ", Temp=" << data_profile_ptr->temperature_celsius << "C"
                          << ", NetLatency=" << data_profile_ptr->network_latency_ms << "ms"
                          << ", NetBw=" << data_profile_ptr->network_bandwidth_mbps << "Mbps"
                          << ", IoThroughput=" << data_profile_ptr->io_throughput_mbps << "Mbps"
                          << ", DataQuality=" << data_profile_ptr->data_quality_score << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "  -> Error fetching/processing IoT data: " << e.what() << std::endl;
                // Keep default values in DataProfile if IoT fetch fails
            }
        } else {
            std::cout << "  -> IoTClient not available, skipping IoT data fetch." << std::endl;
        }
        // --- End of IoT Sensor Data Population ---

        return {data_profile_ptr, task.task_type};
    }

    // Definition for the static helper function to calculate Hamming Weight and Sparsity
    void Cortex::calculate_hamming_weight_for_profile(const void* data, size_t num_bytes, DataProfile& profile)
    {
        if (!data || num_bytes == 0) {
            profile.hamming_weight = 0;
            profile.sparsity_ratio = 1.0; // No bits set means fully sparse
            return;
        }

        uint64_t total_hw = 0;
        const uint8_t* byte_data = static_cast<const uint8_t*>(data);

        for (size_t i = 0; i < num_bytes; ++i) {
        #if defined(__GNUC__) || defined(__clang__)
            total_hw += __builtin_popcount(byte_data[i]);
        #elif defined(_MSC_VER)
            // __popcnt intrinsic in MSVC typically takes unsigned int.
            // For a single byte, we can cast or use a small loop.
            unsigned char byte = byte_data[i];
            total_hw += __popcnt(static_cast<unsigned int>(byte)); // Cast byte to unsigned int
        #else
            // Fallback for other compilers
            unsigned char byte = byte_data[i];
            for(int bit = 0; bit < 8; ++bit) {
                if((byte >> bit) & 1) total_hw++;
            }
        #endif
        }
        profile.hamming_weight = total_hw;

        uint64_t total_bits = num_bytes * 8;
        profile.sparsity_ratio = (total_bits > 0) ? (1.0 - (static_cast<double>(total_hw) / total_bits)) : 1.0;
    }

    // Private method for actual profiling logic
    OmniProfile Cortex::profileOmni(const double* data, int num_elements) { // Renamed class
        OmniProfile p;

        // Basic check for data presence and minimum elements for amplitude flux
        if (!data || num_elements == 0) {
            std::cerr << "Warning: Null data or zero elements provided to profileOmni internal method." << std::endl;
            return p; // Return default profile
        }

        // --- Amplitude Flux Calculation (Example) ---
        // This is a placeholder; actual calculation might be more complex.
        double sum_abs_diff = 0.0;
        if (num_elements > 1) {
            for (int i = 0; i < num_elements - 1; ++i) {
                sum_abs_diff += std::abs(data[i+1] - data[i]);
            }
            p.amplitude_flux = sum_abs_diff / (num_elements - 1);
        } else {
            p.amplitude_flux = 0.0; // Or handle as appropriate for a single element
        }

        // --- Frequency Flux & Entropy Flux Calculation ---
        if (num_elements >= 2) { // Need at least 2 elements for FFT
            fftw_complex* out_complex = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (num_elements / 2 + 1));
            // Cast const away from data for fftw_plan_dft_r2c_1d, this is a common practice with FFTW
            // if the input array is not modified by the library (which is true for r2c transforms).
            fftw_plan plan_r2c = fftw_plan_dft_r2c_1d(num_elements, const_cast<double*>(data), out_complex, FFTW_ESTIMATE);

            if (plan_r2c == NULL || out_complex == NULL) {
                std::cerr << "Warning: FFTW3 plan or memory allocation failed in profileOmni internal method." << std::endl;
                if (out_complex) fftw_free(out_complex);
                // According to FFTW docs, it's safe to call fftw_destroy_plan(NULL)
                fftw_destroy_plan(plan_r2c);
                return p; // Return profile without frequency/entropy flux
            }

            fftw_execute(plan_r2c);

            // Calculate magnitude spectrum
            std::vector<double> magnitude_spectrum(num_elements / 2 + 1);
            double total_magnitude = 0.0;
            for (size_t i = 0; i < (size_t)(num_elements / 2 + 1); ++i) {
                magnitude_spectrum[i] = std::sqrt(out_complex[i][0] * out_complex[i][0] + out_complex[i][1] * out_complex[i][1]);
                total_magnitude += magnitude_spectrum[i];
            }

            // Calculate Frequency Flux (Spectral Centroid)
            // Assumes a sampling rate; for simplicity, normalized frequencies 0 to 0.5 (Nyquist)
            double weighted_sum_freq = 0.0;
            if (total_magnitude > 1e-9) { // Avoid division by zero
                for (size_t i = 0; i < magnitude_spectrum.size(); ++i) {
                    // Normalized frequency for bin i: (i / (num_elements/2)) * 0.5 = i / num_elements
                    // For r2c DFT, there are (N/2 + 1) complex outputs.
                    // The frequencies range from 0 up to Nyquist.
                    // If N is the number of real input points, the k-th output bin corresponds to frequency k/N (for k=0,...,N/2).
                    double normalized_freq = static_cast<double>(i) / num_elements;
                    weighted_sum_freq += normalized_freq * magnitude_spectrum[i];
                }
                p.frequency_flux = weighted_sum_freq / total_magnitude;
            } else {
                p.frequency_flux = 0.0;
            }

            // Calculate Entropy Flux (Spectral Entropy)
            double entropy = 0.0;
            if (total_magnitude > 1e-9) {
                std::vector<double> normalized_spectrum(magnitude_spectrum.size());
                for (size_t i = 0; i < magnitude_spectrum.size(); ++i) {
                    normalized_spectrum[i] = magnitude_spectrum[i] / total_magnitude;
                }

                for (double val : normalized_spectrum) {
                    if (val > 1e-9) { // Avoid log(0)
                        entropy -= val * std::log2(val);
                    }
                }
                // Normalize entropy by log2(number of bins) to get a value between 0 and 1
                if (magnitude_spectrum.size() > 1) {
                     p.entropy_flux = entropy / std::log2(static_cast<double>(magnitude_spectrum.size()));
                } else {
                     p.entropy_flux = 0.0; // Single bin, entropy is 0
                }

            } else {
                p.entropy_flux = 0.0;
            }

            fftw_destroy_plan(plan_r2c);
            fftw_free(out_complex);
        } else {
            // Handle cases with less than 2 elements if FFT cannot be performed
            p.frequency_flux = 0.0;
            p.entropy_flux = 0.0;
        }
        // --- End of Frequency Flux & Entropy Flux Calculation ---

        return p;
    }

} // namespace VPU
