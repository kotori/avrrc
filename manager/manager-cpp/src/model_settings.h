#include <cstdint>

struct __attribute__((packed)) ModelSettings {
    char name[12];
    uint64_t boatAddress; 
    int32_t xMin, xMax, yMin, yMax;
    int32_t trims[4];
};
