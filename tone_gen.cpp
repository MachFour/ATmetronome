#include "defines.h"
#include "tone_gen.h"

#include <avr/interrupt.h>
#include <avr/io.h>

void tone_gen_init() {
	uint8_t oldSREG = SREG;
	cli();

	// clear timer
	TCNT2 = 0;
	// disable timer2 interrupts.
	TIMSK2 = 0;

	// reinitialise control registers. This also stops the timer
	TCCR2A = 0;
	TCCR2B = 0;
	// set toggling of OC2A (Arduino pin 11) on compare match
	bitSet(TCCR2A, COM2A0);
	// put into in CTC mode
	bitSet(TCCR2A, WGM21);

	// PORTB3 is ignored since COM2A0 is set.
	bitSet(TONE_GEN_DDR, TONE_GEN_PIN);

	SREG = oldSREG;
}



void tone_gen_start(ToneConfig toneConfig) {
    // from datasheet, ordered so that the bits in the index are the same
    // as the bits needed to be set in TCCR2B to activate that prescaler

    /*
	// the frequency that would be played if the prescaler
	// were set to 1 and OCR set to zero
    //uint32_t base_frequency = F_CPU/approx_frequency/2;
    // scan through prescalars to find the best fit, with 8-bit count (OCR) value
    uint32_t test_ocr;
    uint16_t selected_prescaler;
	uint8_t prescalar_bits = 0;
	do {
		prescalar_bits++;
		selected_prescaler = prescalars[prescalar_bits];
		test_ocr = base_frequency/selected_prescaler - 1;
	} while (test_ocr > 255 && selected_prescaler != prescalars[num_prescalars-1]);

	// just to be safe
    prescalar_bits &= 0b00000111u;
    auto ocr = static_cast<uint8_t>(test_ocr);
    */

	// set prescaler, and the reset level

    // can't just OR in new bits as we need to clear prescalar bits that should now be 0
    uint8_t new_tccr2b = (uint8_t)(TCCR2B & (uint8_t) 0b11111000u) | toneConfig.prescalar_bits;

    uint8_t oldSREG = SREG;
    cli();
    OCR2A = toneConfig.count_value;
    TCCR2B = new_tccr2b;
	SREG = oldSREG;
}


void tone_gen_stop() {
	uint8_t oldSREG = SREG;
	cli();

	// remove clock source from timer 2, stopping it.
	TCCR2B = 0;
	// set timer count to zero
	TCNT2 = 0;

	// check if the pin is set high. If so, trigger one more output compare
	// to toggle it to zero.
	if (bitRead(PINB, PINB3)) {
		// Force output compare A. Should work even if timer is not running.
		bitSet(TCCR2B, FOC2A);
	}


	SREG = oldSREG;
}
