
#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>


void timer0_start();

void timer0_hold_reset();

void timer0_setup(uint8_t ocr0a, uint8_t ocr0b);


void timer1_start();
void timer1_init();

#endif // TIMERS_H
