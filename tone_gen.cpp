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

	// set DDR of PB3 (Arduino pin 11) as output.
	// PORTB3 is ignored since COM2A0 is set.
	bitSet(DDRB, DDB3);

	SREG = oldSREG;
}

void tone_gen_start(uint16_t frequency) {
	const uint16_t prescaler_values[] = {0, 1, 8, 32, 64, 128, 256, 1024};

	// the frequency that would be played if the prescaler
	// were set to 1 and OCR set to zero
	uint32_t base_frequency = F_CPU/frequency/2;

	uint8_t prescalar_bits = 0;
	uint32_t ocr = 0;

	uint16_t selected_prescaler;

	uint8_t oldSREG = SREG;
	cli();

    // scan through prescalars to find the best fit
	do {
		prescalar_bits++;
		selected_prescaler = prescaler_values[prescalar_bits];
		ocr = base_frequency/selected_prescaler - 1;
	} while (ocr > 255);

	// set prescaler, and the reset level
	TCCR2B = (TCCR2B & 0b11111000) | (prescalar_bits & 0b00000111);
    OCR2A = ocr;

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
