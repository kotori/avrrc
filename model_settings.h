#ifndef MODEL_SETTINGS_H
#define MODEL_SETTINGS_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// --- CROSS-PLATFORM PACKING MACROS ---
#if defined(_MSC_VER)
    #define PACKED_BEGIN __pragma(pack(push, 1))
    #define PACKED_END __pragma(pack(pop))
    #define PACKED_ATTR 
#else
    #define PACKED_BEGIN
    #define PACKED_END
    #define PACKED_ATTR __attribute__((packed))
#endif

/**
 * @brief Unified Data Structure for AVRRC Ensign Fleet Management
 * Total Size: 44 Bytes
 */
PACKED_BEGIN
struct PACKED_ATTR ModelSettings {
    char name[12];         // 12 bytes: Model name
    uint64_t boatAddress;  // 8 bytes: Unique NRF24 pipe ID
    int32_t xMin;          // 4 bytes: Stick calibration
    int32_t xMax;          // 4 bytes: Stick calibration
    int32_t yMin;          // 4 bytes: Stick calibration
    int32_t yMax;          // 4 bytes: Stick calibration
    int32_t trims[4];      // 16 bytes: Digital Trims
};
PACKED_END

#endif // MODEL_SETTINGS_H
