//
// Created by max on 11/18/18.
//

#ifndef METRONOME_HELPER_H
#define METRONOME_HELPER_H

#include <stdint.h>

// easy way to make uint8_t integer literals
inline constexpr uint8_t operator "" _u8(unsigned long long arg) noexcept {
    return static_cast<uint8_t>(arg);
}
// easy way to make uint16_t integer literals
inline constexpr uint16_t operator "" _u16(unsigned long long arg) noexcept {
    return static_cast<uint16_t>(arg);
}

#endif //METRONOME_HELPER_H
