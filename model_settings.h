#ifndef MODEL_SETTINGS_H
#define MODEL_SETTINGS_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

/**
 * @brief Unified Data Structure for AVRRC Fleet Management
 * Total Size: 44 Bytes
 *
 * This struct is shared between the 8-bit AVR (Mega) and 64-bit Linux (Qt).
 * The 'packed' attribute prevents compilers from adding padding bytes.
 */
struct __attribute__((packed)) ModelSettings {
    char name[12];         // 12 bytes: Model name (Max 11 chars + null)
    uint64_t boatAddress;  // 8 bytes: Unique NRF24 pipe ID
    int32_t xMin;          // 4 bytes: Left Stick X calibration
    int32_t xMax;          // 4 bytes: Left Stick X calibration
    int32_t yMin;          // 4 bytes: Left Stick Y calibration
    int32_t yMax;          // 4 bytes: Left Stick Y calibration
    int32_t trims[4];      // 16 bytes: CH1-CH4 Digital Trims
};

#endif // MODEL_SETTINGS_H
