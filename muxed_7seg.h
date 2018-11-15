#ifndef MUXED_7SEG_H
#define MUXED_7SEG_H

#include <stdint.h>
/* Public functions */

/* Sets the given digit to display the given character. 0 refers to the LSB.
 * If there is no sensible way to display the character, all segments will be lit.
 * This condition can be checked using muxed_7seg_is_printable_character()
 * digit_index must be between 0 and MUXED_7SEG_NUM_DIGITS.
 */

void muxed_7seg_set_digit(uint8_t digit_index, char c, bool with_dot);

/*
 * Returns true if the library knows how to display the given character on
 * a 7 segment display.
 */
bool muxed_7seg_is_printable_character(char c);


void muxed_7seg_display_off();
void muxed_7seg_display_on();

/*
 * Digit pins are hardcoded as PORTB 0:2
 * The segment pins are hardcoded as PORTD 0:7
 */
void muxed_7seg_init();

/*
 * Helper function to show a number on the display.
 * Signed numbers can be displayed, but the number of displayable digits
 * is reduced by one, to make space for the negative sign.
 * If more than MUXED_7SEG_NUM_DIGITS are required to show the number,
 * it will be truncated to the available number of LSBs.
 */
void muxed_7seg_show_number(int number, bool hide_leading_zeros);


/* Put in the interrupt handler for the 'high' part of the timer
 * The time between high and low callbacks determines the duty cycle
 * of the digit display segments.
 */
void muxed_7seg_timer_high_callback();
void muxed_7seg_timer_low_callback();

#endif /* MUXED_7SEG_H */
