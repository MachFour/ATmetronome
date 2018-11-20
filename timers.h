
#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>


void timer0_1_start();

void timer0_1_hold_reset();

void timer0_setup(uint8_t ocr0a, uint8_t ocr0b);

#endif // TIMERS_H
