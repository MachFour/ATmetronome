
#include "timers.h"
#include "byte_ops.h"

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>


void timer0_1_start() {
    // release prescaler reset, start timer0 and timer1
	bitClear(GTCCR, TSM);
}

// stop and reset timer
void timer0_1_hold_reset() {
	bitSet(GTCCR, TSM);
	bitSet(GTCCR, PSRSYNC);
	TCNT0 = 0;
	TCNT1 = 0;
}

/*
 * Sets up timer 0 to have interrupts fire each time the counter reaches either
 * of two values which are placed in the OCR0{A,B} registers.
 *
 * Since this is the timer used to realise the Arduino library's delay(),
 * millis(), and associated functions, the operation of the timer is kept
 * compatible with how it is set by the library. In particular, the timer
 * always counts up to its maximum value (255) before being reset to zero, and
 * firing the overflow interrupt that updates the Arduino library's timing
 * variables.

 * Therefore, the prescaler is set to 64, which makes the counter increment
 * at F_CPU/64 Hz. Interrupts on compare match 0A and 0B, and overflow are enabled.
 * The functions called by the interrupt vectors for any of the three possible
 * interrupts will all be called every 64/F_CPU seconds (8us for F_CPU =
 * 8000000)
 *
 * Neither the timer count nor the prescaler is reset by this function
 */
void timer0_setup(uint8_t ocr0a, uint8_t ocr0b) {
	/*
	 * TCCR0A
	 *  Bit 7 |        |        |        |        |        |        |  Bit 0 |
	 * COM0A1 | COM0A0 | COM0B1 | COM0B0 |        |        |  WGM01 |  WGM00 |
	 *
	 * TCCR0B
	 *  Bit 7 |        |        |        |        |        |        |  Bit 0 |
	 *  FOC0A |  FOC0B |        |        |  WGM02 |         CS0[2:0]         |
	 *
	 * COM2A, COM2B control output compare pin behaviour for matches with
	 * compare registers OCR2A, OCR2B.  Since we do not use the output
	 * compare pin, these values are set to 0.
	 *
	 * WGM0 controls the waveform generation mode of Timer 0.
	 * To preserve compatibility with Arduino's millis() and delay() et. al.
	 * functions, which require the timer to overflow and generate an interrupt
	 * every 256 ticks, we keep the waveform generation mode in mode 0 (normal)
	 * by setting WGM0[2:0] = 0b000.
	 *
	 * FOC0 are write-only bits (always read as zero) that manually trigger an
	 * output compare match on the respective OC0 pins. The actual event that
	 * occurs is determined by the COM0 pins above.
	 *
	 * CS0 is the clock source selection, which can be used to either stop
	 * Timer0 or set the prescaler.
	 * We set the prescaler to 64, to preserve compatibility with the Arduino
	 * library. This means setting CS0[2:0] = 0b011. This means that the
	 * timer counts at F_CPU/64 Hz. If the CPU frequency is 8MHz, each count
	 * of the timer thus takes 64/8000000 seconds, or 8 us. This is the
	 * granularity of the timer compare actions.
	 *
	 * TIMSK0
	 *  Bit 7 |        |        |        |        |        |        |  Bit 0 |
	 *        |        |        |        |        | OCIE0B | OCIE0A |  TOIE0 |
	 *
	 * These bits control whether interrupts are generated for the
	 * corresponding Output Compare Match events, or the timer overflow event
	 * for TOIE. These events occur (by definition) when the OCFB, OCFA or TOV
	 * bits are respectively set in TIFR2.
	 *
	 * GTCCR
	 *  Bit 7 |        |        |        |        |        |        |  Bit 0 |
	 *    TSM |        |        |        |        |        | PSRASY | PSRSYNC|
	 *
	 * PSRASY and PSRSYNC are used to reset the prescalers of Timer 2 and
	 * Timers 0/1 respectively. Writing 1 to these bits does so, and the bit is
	 * cleared immediately by hardware.
	 * Writing TSM to one beforehand, however, locks the values of the above
	 * bits, so that the timers can be held in reset and configured.
	 *
	 * TIFR0
	 *  Bit 7 |        |        |        |        |        |        |  Bit 0 |
	 *        |        |        |        |        | OC0FB  | OC0FA  |  TOV0  |
	 * This register holds the interrupt flags for the Output Compare Match
	 * (A, B) and Timer 0 Overflow interrupts. They are set to 1 whenever an
	 * interrupt occurs, and also cleared by writing a 1 to the flag.
	 * To have the corresponding interrupt fire, the bit in this register, the
	 * I-bit in SREG, and the corresponding interrupt enable bit (OCIE0A,
	 * OCIE0B, TOIE0) must all be set to 1.
	 *
	 * TCNT0
	 * is the raw count value of timer 0 (8 bits).
	 */

	auto sreg = SREG;
	cli();

	// set timer behaviour
	TCCR0A = 0;
	TCCR0B = 0;

	bitSet(TCCR0B, CS01);
	bitSet(TCCR0B, CS00);

	// set interrupt phases
	OCR0A = ocr0a;
	OCR0B = ocr0b;

	// clear previous interrupt flags (shouldn't really be necessary)
	/*
	bitSet(TIFR0, OCF0B);
	bitSet(TIFR0, OCF0A);
	bitSet(TIFR0, TOV0);
	*/

	// enable interrupts on output compare matches A and B, and on overflow.
	TIMSK0 = 0;
	bitSet(TIMSK0, OCIE0B);
	bitSet(TIMSK0, OCIE0A);
	bitSet(TIMSK0, TOIE0);

	SREG = sreg;
}

