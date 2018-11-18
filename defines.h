
#ifndef DEFINES_H
#define DEFINES_H

#define interrupts() sei()
#define noInterrupts() cli()

/* Muxed 7 segment display ports/pins
 * The whole of PORTD is used for segment control, while PORTB 0-2 are
 * used for digit control
 */
#define DIGIT_PORT PORTB
#define DIGIT_DDR DDRB
#define SEGMENT_DDR DDRD
#define SEGMENT_PORT PORTD


/* Inputs/Switches */

#define INPUT_DDR DDRC
#define INPUT_PORT PORTC
// control switch
#define SWITCHC PORTC0
// down switch
#define SWITCHD PORTC1
// up switch
#define SWITCHU PORTC2

#define LED_PORT PORTB
#define LED_PIN PORTB5
#define LED_DDR DDRB

// can't use this for IO as it's being taken by OC2B
#define TONE_GEN_PORT PORTB
#define TONE_GEN_DDR DDRB
#define TONE_GEN_PIN PORTB3


#define bitRead(value, bit) (((value) >> (unsigned char)(bit)) != 0)
#define bitSet(value, bit) ((value) |= (1u << (unsigned char)(bit)))
#define bitClear(value, bit) ((value) &= ~(1u << (unsigned char)(bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define delay(x) _delay_ms(x)

#endif //DEFINES_H
