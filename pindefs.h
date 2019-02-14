//
// Created by max on 11/21/18.
//

#ifndef METRONOME_PINDEFS_H
#define METRONOME_PINDEFS_H

#include <avr/io.h>

/* Muxed 7 segment display ports/pins
 * The whole of PORTD is used for segment control, while PORTB 0-2 are
 * used for digit control
 */
#define DIGIT_PORT PORTB
#define DIGIT_DDR DDRB
#define SEGMENT_DDR DDRD
#define SEGMENT_PORT PORTD

#define MUXED_7SEG_NUM_DIGITS 3
#define DIGIT_0 PORTB0
#define DIGIT_1 PORTB1
#define DIGIT_2 PORTB2
static constexpr unsigned char digit_pin[] {DIGIT_0, DIGIT_1, DIGIT_2};


/* Inputs/Switches */

#define INPUT_DDR DDRC
#define INPUT_REGISTER PINC
#define INPUT_PORT PORTC
// control switch
#define SWITCHC PORTC0
// down switch
#define SWITCHD PORTC1
// up switch
#define SWITCHU PORTC2
// stop/start switch
#define SWITCHS PORTC3

#define LED_PORT PORTB
#define LED_PIN PORTB5
#define LED_DDR DDRB

// can't use this for IO as it's being taken by OC2B
#define TONE_GEN_PORT PORTB
#define TONE_GEN_DDR DDRB
#define TONE_GEN_PIN PORTB3

#endif //METRONOME_PINDEFS_H
