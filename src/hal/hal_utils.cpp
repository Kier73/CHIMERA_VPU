#include "hal_utils.h"

// Required for MSVC __popcnt if not included elsewhere,
// and for GCC/Clang __builtin_popcount if not implicitly available.
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace VPU {
namespace HAL {

uint64_t calculate_data_hamming_weight(const void* data, size_t bytes) {
    if (!data || bytes == 0) return 0;
    const uint8_t* byte_data = static_cast<const uint8_t*>(data);
    uint64_t total_hw = 0;
    for (size_t i = 0; i < bytes; ++i) {
#if defined(__GNUC__) || defined(__clang__)
        total_hw += __builtin_popcount(byte_data[i]);
#elif defined(_MSC_VER)
        total_hw += __popcnt(static_cast<unsigned int>(byte_data[i])); // __popcnt needs unsigned int for some versions
#else
        // Fallback for other compilers
        unsigned char current_byte = byte_data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if ((current_byte >> bit) & 1) total_hw++;
        }
#endif
    }
    return total_hw;
}

} // namespace HAL
} // namespace VPU
