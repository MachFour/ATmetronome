
#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#include <avr/io.h>

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

#define MUXED_7SEG_NUM_DIGITS 3
#define DIGIT_0 PORTB0
#define DIGIT_1 PORTB1
#define DIGIT_2 PORTB2
static constexpr uint8_t digit_pin[] {DIGIT_0, DIGIT_1, DIGIT_2};


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

#define LED_PORT PORTB
#define LED_PIN PORTB5
#define LED_DDR DDRB

// can't use this for IO as it's being taken by OC2B
#define TONE_GEN_PORT PORTB
#define TONE_GEN_DDR DDRB
#define TONE_GEN_PIN PORTB3

/*
 * UNSIGNED BYTE
 */

// easy way to make uint8_t integer literals
inline constexpr uint8_t operator "" _u8(unsigned long long arg) noexcept {
    return static_cast<uint8_t>(arg);
}
// easy way to make uint16_t integer literals
inline constexpr uint16_t operator "" _u16(unsigned long long arg) noexcept {
    return static_cast<uint16_t>(arg);
}


// get rid of clang-tidy's warning about unsigned bitwise operations
#define bitMask(bit) ((u8)(1_u8 << (u8)(bit)))
#define byteInverse(byte) (u8)(~(byte))

#define byteOr(b1, b2) (u8) ((b1) | (b2))
#define byteOr3(b1, b2, b3) byteOr(byteOr(b1, b2), b3)

#define mask1(bit1) bitMask(bit1)
#define mask2(bit1, bit2) byteOr(bitMask(bit1), bitMask(bit2))
#define mask3(bit1, bit2, bit3) byteOr3(bitMask(bit1), bitMask(bit2), bitMask(bit3))


typedef uint8_t u8;

inline void _bitSet(volatile u8 * address, u8 bit) {
    *address |= mask1(bit);
}

inline void _bitSet(volatile u8 * address, u8 bit1, u8 bit2) {
    *address |= mask2(bit1, bit2);
}

inline void _bitSet(volatile u8 * address, u8 bit1, u8 bit2, u8 bit3) {
    *address |= mask3(bit1, bit2, bit3);
}

inline void _bitClear(volatile u8 * address, u8 bit) {
    *address &= byteInverse(mask1(bit));
}

inline void _bitClear(volatile u8 * address, u8 bit1, u8 bit2) {
    *address &= byteInverse(mask2(bit1, bit2));
}

inline void _bitClear(volatile u8 * address, u8 bit1, u8 bit2, u8 bit3) {
    *address |= byteInverse(mask3(bit1, bit2, bit3));
}

inline bool _bitRead(volatile const u8 * address, u8 bit) {
    return (*address & bitMask(bit)) != 0;
}

#define bitRead(value, bit) _bitRead(&(value), (bit))
#define bitSet(value, ...) _bitSet(&(value), __VA_ARGS__)
#define bitClear(value, ...) _bitClear(&(value), __VA_ARGS__)

// from https://www.avrfreaks.net/forum/8-bit-bitwise-ops-without-casting
// ~2003 workaround to avoid promotion of 8-bit values to 16-bit ints
#if 0
#define bit_set_byte(var, mask) ((uint8_t)(var) |= (uint8_t)(mask))
#define bit_clear_byte(var, mask) ((uint8_t)(var) &= (uint8_t)~(mask))
#define bit_toggle_byte(var, mask) ((uint8_t)(var) ^= (uint8_t)(mask))
#define bit_is_set_byte(var, bit) ((uint8_t)(var) & (uint8_t)_BV(bit))
#define bit_is_clear_byte(var, bit) (!bit_is_set_byte(var, bit))
#define loop_until_bit_is_set_byte(var, bit) do{}while(bit_is_clear_byte(var, bit))
#define loop_until_bit_is_clear_byte(var, bit) do{}while(bit_is_set_byte(var, bit))

#define bit_set_word(var, mask) ((uint16_t)(var) |= (uint16_t)(mask))
#define bit_clear_word(var, mask) ((uint16_t)(var) &= (uint16_t)~(mask))
#define bit_toggle_word(var, mask) ((uint16_t)(var) ^= (uint16_t)(mask))
#define bit_is_set_word(var, bit) ((uint16_t)(var) & (uint16_t)_BV(bit))
#define bit_is_clear_word(var, bit) (!bit_is_set_word(var, bit))
#define loop_until_bit_is_set_word(var, bit) do{}while(bit_is_clear_word(var, bit))
#define loop_until_bit_is_clear_word(var, bit) do{}while(bit_is_set_word(var, bit))
#endif

#endif //DEFINES_H
