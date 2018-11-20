#ifndef TONE_GEN_H
#define TONE_GEN_H

#include <stdint.h>

struct ToneConfig {
    uint8_t prescalar_bits;
    uint8_t count_value;
};

void tone_gen_init();
void tone_gen_start(ToneConfig toneConfig);
void tone_gen_stop();

/* Precompute OCR values */
constexpr ToneConfig tone_gen_makeConfig(uint16_t approximate_frequency) {

    constexpr uint16_t prescalars[] = {0, 1, 8, 32, 64, 128, 256, 1024};
    constexpr uint8_t num_prescalars = sizeof(prescalars)/sizeof(prescalars[0]);

    uint32_t base_frequency = F_CPU/approximate_frequency/2;

    // find smallest prescalar that gives a count value <= 255
    for (uint8_t index = num_prescalars - 1_u8; index > 1_u8; --index) {
        uint16_t prescalar = prescalars[index];
        uint16_t next_prescalar_bits = index - 1_u8;
        uint16_t next_prescalar = prescalars[next_prescalar_bits];
        if (base_frequency / next_prescalar > 255) {
            // next prescalar is too fast to fit the count value in 8 bits
            uint8_t count_value = ((uint8_t)(base_frequency / prescalar)) - 1_u8;
            // index coincides with bits of prescalar
            return {index, count_value};
        }
    }
    // is it even possible to get here?
    return {0, 0};
}

#endif // TONE_GEN_H
