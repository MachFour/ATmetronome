//
// Created by max on 11/18/18
// millis() functionality copied from Arduino project's wiring.c
//

#ifndef METRONOME_MILLIS_H
#define METRONOME_MILLIS_H

#include <stdint.h>

void millis_timer0_callback();
// measures milliseconds since timer0 started
uint32_t millis();
// counts microseconds, but periodically overflows
uint32_t micros();

void delay(uint32_t ms);

// need this to actually make it all work
//ISR(TIMER0_OVF_vect) {
// millis_timer0_callback();
//}
#endif //METRONOME_MILLIS_H
