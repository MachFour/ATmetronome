
/*
 * Firmware to drive a time-multiplexed seven segment display
 * Created by max on 11/21/18.
 */

#ifndef METRONOME_SEVENSEG_H
#define METRONOME_SEVENSEG_H

#include "byte_ops.h"
#include "pindefs.h"
#include <avr/io.h>

class SevenSeg {
private:
    /*
     * Which digit of the display is currently lit (for multiplexing purposes)
     * Should always be between 0 and NUM_DIGITS-1.
     * Updated (incremented) each timer subBeat
     */
    uint8_t currentDigit;
    /*
     * Holds the currently displayed segment data for each digit.
     * Note that the segments have to be negated as compared to how they in the
     * SEGMENT_DATA table, since the segment pins function to ground the LEDs
     */
    uint8_t segmentData[MUXED_7SEG_NUM_DIGITS];

    bool enabled;

public:

    SevenSeg() noexcept: currentDigit(0), segmentData{0}, enabled(false) {}
    /* Sets the given digit to display the given character. 0 refers to the LSB.
     * If there is no sensible way to display the character, all segments will be lit.
     * This condition can be checked using muxed_7seg_is_printable_character()
     * digit_index must be between 0 and MUXED_7SEG_NUM_DIGITS.
     */
    void setDigit(uint8_t digit, char c, bool withDot);

    /*
     * Returns true if the library knows how to display the given character on
     * a 7 segment display.
     */
    static bool isPrintableChar(char c);

    /*
     * Just disables or enables the actual display of characters.
     * Doesn't modify any stored data
     */
    void displayOff();
    void displayOn();

    /*
     * Digit pins are hardcoded as PORTB 0:2
     * The segment pins are hardcoded as PORTD 0:7
     */
    void setup();

    /*
     * Helper function to show a number on the display.
     * Signed numbers can be displayed, but the number of displayable digits
     * is reduced by one, to make space for the negative sign.
     * If more than MUXED_7SEG_NUM_DIGITS are required to show the number,
     * it will be truncated to the available number of LSBs.
     */
    void showNumber(int number, bool hide_leading_zeros);

    /* Used to control the multiplexing of the display.
     * These functions should be called by (fast) timer interrupt routines.
     * The duty cycle of the high and low callbacks determines the
     * brightness of the display.
     */
    void timerHighCallback();
    void timerLowCallback();

private:
    void cycleDigit();
    void switchOnActiveDigit();
    void switchOffActiveDigit();
};

#endif //METRONOME_SEVENSEG_H
