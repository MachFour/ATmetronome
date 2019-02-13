#ifndef TONE_GEN_H
#define TONE_GEN_H

#include "byte_ops.h"

/*
 * Tone generator class, which is hardcoded to use Timer 2 on the ATMega328p
 */

class ToneGen {
    struct Config {
        uint8_t prescalar_bits;
        uint8_t count_value;
    };
public:
    void setup();
    void start(Config c);
    void stop();
    /*
     * Computes prescalar bits and timer count value for a given approximate frequency
     */
    static constexpr Config makeConfig(uint16_t approxFrequency) {
        /* Source: Atmega328p datasheet
         * The following array is constructed so that the index of each
         * prescalar, when expressed in binary, coincides with the
         * control bit pattern used to select it
         */
        constexpr uint16_t prescalars[] = {0, 1, 8, 32, 64, 128, 256, 1024};
        constexpr uint8_t num_prescalars = sizeof(prescalars)/sizeof(prescalars[0]);

        uint32_t base_frequency = F_CPU/approxFrequency/2;

        // find smallest prescalar that gives a count value <= 255
        for (uint8_t index = num_prescalars - 1_u8; index > 1_u8; --index) {
            auto prescalar = prescalars[index];
            auto next_prescalar_bits = index - 1_u8;
            auto next_prescalar = prescalars[next_prescalar_bits];
            if (base_frequency / next_prescalar > 255) {
                // next prescalar is too small to fit the count value in 8 bits,
                // so we are done
                uint8_t count_value = ((uint8_t)(base_frequency / prescalar)) - 1_u8;
                return {index, count_value};
            }
        }
        // is it even possible to get here?
        return {0, 0};
    }

};

#endif // TONE_GEN_H
