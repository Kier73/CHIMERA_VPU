#include "core/Pillar2_Cortex.h" // Corresponding header
#include <vector>
#include <cmath>        // For std::sqrt, std::log2, std::abs
#include <numeric>      // For std::accumulate (if needed elsewhere)
#include <iostream>     // For std::cerr
#include <algorithm>    // For std::fill (if needed elsewhere)

// The actual fftw3.h is already included in Pillar2_Cortex.h
// No need for simulated FFTW functions or typedefs here.

namespace VPU { // Changed namespace

    Cortex::Cortex() { // Renamed class
        // Constructor implementation (if any)
        std::cout << "[Pillar 2] Cortex initialized." << std::endl;
    }

    Cortex::~Cortex() { // Renamed class
        // Destructor implementation (if any)
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
        // TODO: Populate hamming_weight and sparsity_ratio from VPU_Task if applicable/available.
        // For example, if task_type indicates binary data, or if VPU_Task has fields for these.

        std::cout << "  -> OmniProfile generated: AF=" << data_profile_ptr->amplitude_flux
                  << ", FF=" << data_profile_ptr->frequency_flux
                  << ", EF=" << data_profile_ptr->entropy_flux << std::endl;

        return {data_profile_ptr, task.task_type};
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
