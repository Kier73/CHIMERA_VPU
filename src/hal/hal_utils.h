#pragma once

#include <cstddef> // For size_t
#include <cstdint> // For uint64_t, uint8_t

namespace VPU {
namespace HAL {

// Helper to calculate Hamming weight of a raw data buffer
// Used by kernel wrappers to report hw_in_cost and hw_out_cost
uint64_t calculate_data_hamming_weight(const void* data, size_t bytes);

} // namespace HAL
} // namespace VPU
