/*
 * Implementation of functions for driving a time-multiplexed 7 segment
 * display, using an AVR ATmega328p.
 * Created by max on 11/21/18.
 */

#include "SevenSeg.h"

#include "byte_ops.h"
#include "pindefs.h"

#include <avr/io.h>
#include <avr/interrupt.h>


/* current wiring
 * i.e. to make the given segment light up,
 * make the nth bit a one when shifting in 7 bits
   +=== 1 ===+
   |         |
   8         2
   |         |
   +=== 7 ===+
   |         |
   6         3
   |         |
   +=== 5 ===+ (4)
*/

// ASCII table for 7 seg display
// Illegal character lights everything up
const static uint8_t NOT_DISPLAYABLE = 0b00000000;

const static uint8_t SEGMENT_DATA[128] = {
        NOT_DISPLAYABLE, // ASCII 0
        NOT_DISPLAYABLE, // ASCII 1
        NOT_DISPLAYABLE, // ASCII 2
        NOT_DISPLAYABLE, // ASCII 3
        NOT_DISPLAYABLE, // ASCII 4
        NOT_DISPLAYABLE, // ASCII 5
        NOT_DISPLAYABLE, // ASCII 6
        NOT_DISPLAYABLE, // ASCII 7
        NOT_DISPLAYABLE, // ASCII 8
        NOT_DISPLAYABLE, // ASCII 9
        NOT_DISPLAYABLE, // ASCII 10
        NOT_DISPLAYABLE, // ASCII 11
        NOT_DISPLAYABLE, // ASCII 12
        NOT_DISPLAYABLE, // ASCII 13
        NOT_DISPLAYABLE, // ASCII 14
        NOT_DISPLAYABLE, // ASCII 15
        NOT_DISPLAYABLE, // ASCII 16
        NOT_DISPLAYABLE, // ASCII 17
        NOT_DISPLAYABLE, // ASCII 18
        NOT_DISPLAYABLE, // ASCII 19
        NOT_DISPLAYABLE, // ASCII 20
        NOT_DISPLAYABLE, // ASCII 21
        NOT_DISPLAYABLE, // ASCII 22
        NOT_DISPLAYABLE, // ASCII 23
        NOT_DISPLAYABLE, // ASCII 24
        NOT_DISPLAYABLE, // ASCII 25
        NOT_DISPLAYABLE, // ASCII 26
        NOT_DISPLAYABLE, // ASCII 27
        NOT_DISPLAYABLE, // ASCII 28
        NOT_DISPLAYABLE, // ASCII 29
        NOT_DISPLAYABLE, // ASCII 30
        NOT_DISPLAYABLE, // ASCII 31
        NOT_DISPLAYABLE, // ASCII 32
        0b01110000,      // ASCII 33 '!'
        NOT_DISPLAYABLE, // ASCII 34 '"'
        NOT_DISPLAYABLE, // ASCII 35 '#'
        NOT_DISPLAYABLE, // ASCII 36 '$'
        NOT_DISPLAYABLE, // ASCII 37 '%'
        NOT_DISPLAYABLE, // ASCII 38 '&'
        0b01000000,      // ASCII 39 '''
        NOT_DISPLAYABLE, // ASCII 40 '('
        NOT_DISPLAYABLE, // ASCII 41 ')'
        NOT_DISPLAYABLE, // ASCII 42 '*'
        NOT_DISPLAYABLE, // ASCII 43 '+'
        NOT_DISPLAYABLE, // ASCII 44 ','
        0b00000010,      // ASCII 45 '-'
        0b00010000,      // ASCII 46 '.'
        NOT_DISPLAYABLE, // ASCII 47 '/'
        0b11101101,      // ASCII 48 '0'
        0b01100000,      // ASCII 49 '1'
        0b11001110,      // ASCII 50 '2'
        0b11101010,      // ASCII 51 '3'
        0b01100011,      // ASCII 52 '4'
        0b10101011,      // ASCII 53 '5'
        0b10101111,      // ASCII 54 '6'
        0b11100000,      // ASCII 55 '7'
        0b11101111,      // ASCII 56 '8'
        0b11101011,      // ASCII 57 '9'
        NOT_DISPLAYABLE, // ASCII 58 ':'
        NOT_DISPLAYABLE, // ASCII 59 ';'
        NOT_DISPLAYABLE, // ASCII 60 '<'
        0b10000010,      // ASCII 61 '='
        NOT_DISPLAYABLE, // ASCII 62 '>'
        0b11000110,      // ASCII 63 '?'
        NOT_DISPLAYABLE, // ASCII 64 '@'
        0b11100111,      // ASCII 65 'A'
        0b11101111,      // ASCII 66 'B' // same as '8'
        0b10001101,      // ASCII 67 'C'
        NOT_DISPLAYABLE, // ASCII 68 'D'
        0b10001111,      // ASCII 69 'E'
        0b10000111,      // ASCII 70 'F'
        0b10101101,      // ASCII 71 'G'
        0b01100111,      // ASCII 72 'H'
        0b00000101,      // ASCII 73 'I' // same as 'l'
        0b01101100,      // ASCII 74 'J'
        NOT_DISPLAYABLE, // ASCII 75 'K'
        0b00001101,      // ASCII 76 'L'
        NOT_DISPLAYABLE, // ASCII 77 'M'
        0b11100101,      // ASCII 78 'N'
        0b11101101,      // ASCII 79 'O' // same as '0'
        0b11000111,      // ASCII 80 'P'
        NOT_DISPLAYABLE, // ASCII 81 'Q'
        NOT_DISPLAYABLE, // ASCII 82 'R'
        0b10101011,      // ASCII 83 'S' // same as '5'
        NOT_DISPLAYABLE, // ASCII 84 'T'
        0b01101101,      // ASCII 85 'U'
        NOT_DISPLAYABLE, // ASCII 86 'V'
        NOT_DISPLAYABLE, // ASCII 87 'W'
        NOT_DISPLAYABLE, // ASCII 88 'X'
        NOT_DISPLAYABLE, // ASCII 89 'Y'
        NOT_DISPLAYABLE, // ASCII 90 'Z'
        0b10001101,      // ASCII 91 '['
        NOT_DISPLAYABLE, // ASCII 92 '\'
        0b11101000,      // ASCII 93 ']'
        NOT_DISPLAYABLE, // ASCII 94 '^'
        0b00001000,      // ASCII 95 '_'
        0b01000000,      // ASCII 96 '`' // same as '''
        NOT_DISPLAYABLE, // ASCII 97 'a'
        0b00101111,      // ASCII 98 'b'
        0b00001110,      // ASCII 99 'c'
        0b01101110,      // ASCII 100 'd'
        NOT_DISPLAYABLE, // ASCII 101 'e'
        NOT_DISPLAYABLE, // ASCII 102 'f'
        0b11101011,      // ASCII 103 'g' // same as '9'
        0b00100111,      // ASCII 104 'h'
        NOT_DISPLAYABLE, // ASCII 105 'i'
        0b01101000,      // ASCII 106 'j'
        NOT_DISPLAYABLE, // ASCII 107 'k'
        NOT_DISPLAYABLE, // ASCII 108 'l'
        NOT_DISPLAYABLE, // ASCII 109 'm'
        NOT_DISPLAYABLE, // ASCII 110 'n'
        0b00101110,      // ASCII 111 'o'
        0b11000111,      // ASCII 112 'p'
        0b11100011,      // ASCII 113 'q'
        0b00000110,      // ASCII 114 'r'
        0b10101011,      // ASCII 115 's' // same as 'S'
        0b00001111,      // ASCII 116 't'
        0b00101100,      // ASCII 117 'u'
        NOT_DISPLAYABLE, // ASCII 118 'v'
        NOT_DISPLAYABLE, // ASCII 119 'w'
        NOT_DISPLAYABLE, // ASCII 120 'x'
        0b01101011,      // ASCII 121 'y'
        NOT_DISPLAYABLE, // ASCII 122 'z'
        NOT_DISPLAYABLE, // ASCII 123 '{'
        0b00000101,      // ASCII 124 '|'
        NOT_DISPLAYABLE, // ASCII 125 '}'
        NOT_DISPLAYABLE, // ASCII 126 '~'
        NOT_DISPLAYABLE  // ASCII 127
};

bool SevenSeg::isPrintableChar(char c) {
    return SEGMENT_DATA[(uint8_t)c] != NOT_DISPLAYABLE;
}

void SevenSeg::setDigit(uint8_t digit, char c, bool withDot) {
    if (digit >= MUXED_7SEG_NUM_DIGITS) {
        // out of range
        return;
    }

    auto c_idx = static_cast<uint8_t>(c);
    auto dot = static_cast<uint8_t>('.');
    auto segments = SEGMENT_DATA[c_idx];
    if (withDot) {
        segments |= SEGMENT_DATA[dot];
    }
    // actual segment data has to be negated since segment pins act to ground LEDs
    segmentData[digit] = byteInverse(segments);
}

/* Switches on the display. */
void SevenSeg::displayOn() {
    enabled = true;
}


void SevenSeg::setup() {
    bitSet(DIGIT_DDR, DIGIT_0, DIGIT_1, DIGIT_2);
    // fixed PORTD for segment control
    SEGMENT_DDR = 0b11111111;
}

void SevenSeg::cycleDigit() {
    // wrap-around increment of digit index
    currentDigit++;
    if (currentDigit == MUXED_7SEG_NUM_DIGITS) {
        currentDigit = 0;
    }
}


void SevenSeg::showNumber(int number, bool hide_leading_zeros) {
    uint8_t available_digits = MUXED_7SEG_NUM_DIGITS;

    if (number < 0) {
        // display a negative sign. Number of available digits is reduced by 1.
        setDigit(MUXED_7SEG_NUM_DIGITS - 1, '-', false);
        available_digits--;
        // set number to its unsigned equivalent.
        // XXX If number is -2^(n-1), where n is the number of bits in an int,
        // taking its negative will give 0.
        number = - number;
    }

    for (uint8_t i = 0; i < available_digits; i++) {
        char next_rightmost_char;

        if (number == 0 && i > 0 && hide_leading_zeros) {
            next_rightmost_char = ' ';
        } else {
            next_rightmost_char = '0' + (char)(number % 10);
        }

        setDigit(i, next_rightmost_char, false);
        number = number / 10;
    }
}

void SevenSeg::switchOnActiveDigit() {
    bitSet(DIGIT_PORT, digit_pin[currentDigit]);
    SEGMENT_PORT = segmentData[currentDigit];
}

void SevenSeg::switchOffActiveDigit() {
    bitClear(DIGIT_PORT, currentDigit);
    SEGMENT_PORT = 0;
}

/* Switches off the display */
void SevenSeg::displayOff() {
    enabled = false;
    switchOffActiveDigit();
    currentDigit = 0; // this is probably not needed
}

void SevenSeg::timerHighCallback() {
    if (enabled) {
        cycleDigit();
        switchOnActiveDigit();
    }
}
void SevenSeg::timerLowCallback() {
    if (enabled) {
        switchOffActiveDigit();
    }
}
