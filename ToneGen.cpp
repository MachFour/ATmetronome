#include "byte_ops.h"
#include "pindefs.h"
#include "ToneGen.h"

#include <avr/interrupt.h>
#include <avr/io.h>

void ToneGen::setup() {
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

void ToneGen::start(ToneGen::Config c) {
    auto new_tccr2b = byteOr(TCCR2B & 0xf8u, c.prescalar_bits);
    auto oldSREG = SREG;
    cli();
    OCR2A = c.count_value;
    TCCR2B = new_tccr2b;
    SREG = oldSREG;
}

void ToneGen::stop() {
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
