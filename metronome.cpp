#include "metronome.h"
// kludge to get stuff to work together
#include "muxed_7seg.h"
#include "timers.h"
#include "tone_gen.h"
#include "defines.h"

#include <util/delay.h>
#include <avr/io.h>
// for the system clock prescale stuff
#include <avr/power.h>
#include <avr/interrupt.h>
#include <stdint.h>


// negates value, since pins are configured as INPUT_PULLUP
inline static bool pressed(uint8_t input_pin) {
	return !bitRead(INPUT_PORT, input_pin);
}

inline static void led_on() {
    bitSet(LED_PORT, LED_PIN);
}

inline static void led_off() {
    bitClear(LED_PORT, LED_PIN);
}

inline static void led_setup() {
    bitSet(LED_DDR, LED_PIN);
}

/**
 * Displays the BPM on the 7 segment displays
 */
inline static void display_bpm() {
	muxed_7seg_show_number(bpm, false);
    displaying_bpm = true;
}

/**
 * Displays beats_per_measure and ticks_per_beat
 */
static void display_timesig() {
	muxed_7seg_set_digit(2, '0' + (char)(beats_per_measure / 10), WITHOUT_DOT);
	muxed_7seg_set_digit(1, '0' + (char)(beats_per_measure % 10), WITH_DOT);
	muxed_7seg_set_digit(0, '0' + (char)(ticks_per_beat), WITHOUT_DOT);
    displaying_bpm = false;
}

/**
 * Calls a function when when a button press is detected
 * implements ability to hold down a button to automatically repeat the action
 * at the desired repeat rate (in ms)
 */
void process_input(uint8_t input_pin, void (*action)(), uint8_t repeat_rate) {
	action();
	// pause to allow single stepping
	const long current_time = millis();
	while (pressed(input_pin) && millis() - current_time < INCREMENT_REPEAT_DELAY) {
		delay(10);
	}
	// otherwise repeat action at repeat rate
	while (pressed(input_pin)) {
		action();
		delay(repeat_rate);
	}
}

/*
 * Adds the given increment to the specified counter, ensuring that the count
 * remains within the interval [low, high].
 * Have to ensure that the counter does not overflow due to the increment
 * before applying the [low, high] limits.
 */
inline void increment_value(uint8_t *counter, uint8_t increment, uint8_t low, uint8_t high) {
	uint8_t range = high - low + 1;
	uint8_t incremented_counter = *counter + increment;
	// put back into range
	if (incremented_counter < low) {
		incremented_counter += range;
	} else if (incremented_counter > high) {
		incremented_counter -= range;
	}

	*counter = incremented_counter;
}

/* increases or decreases the stored value for bpm by 1, keeping it within range,
 * displays it on the 7 segment displays, and also adjusts the Timer1 compare register
 * to trigger the TIM1_COMPA interrupt at the corresponding frequency.
 */
void increment_bpm(const uint8_t increment) {
	increment_value(&bpm, increment, SOFT_MIN_BPM, SOFT_MAX_BPM);
    update_bpm();
    display_bpm();
}

void increment_bpm() {
	increment_bpm(1u);
}

void decrement_bpm() {
	// this will technically overflow, but it's okay, despite the warning on
	// increment_value()
	increment_bpm((uint8_t) -1);
}

void increment_beats() {
	increment_value(&beats_per_measure, 1, MIN_BEATS_PER_MEASURE, MAX_BEATS_PER_MEASURE);
	display_timesig();
}

void increment_ticks() {
	increment_value(&ticks_per_beat, 1, MIN_TICKS_PER_BEAT, MAX_TICKS_PER_BEAT);
	display_timesig();
}

inline static void metronome_beat() {
	if (beat_num == 0 && beats_per_measure > 0) {
		// start of measure
		tone_gen_start(BEEP_FREQ_MEASURE);

		led_on();
	} else {
		// next beat
		tone_gen_start(BEEP_FREQ_BEAT);
	}
	beat_num++;
	// greater than or equal to to deal with beats_per_measure = 0
	if (beat_num >= beats_per_measure) {
		beat_num = 0;
	}
}

/*
 * Note: OCR1A should be 1 less than the desired count period,
 * since it resets to 0
 */
inline static uint16_t calc_OCR1A() {
    if (tock_num_modulo_bpm < tock_period_remainder) {
        // if there is a rounding down error in tock_period, we should
        // initially count by 1 extra, then go back to tock_period_floor - 1.
        return tock_period_floor; // - 1 + 1;
    } else {
        // Have 'overcounted' enough OR tock_period is exactly divided
        return tock_period_floor - 1;
    }
}

/* writes the appropriate value to OCR1A so that timer 1 resets with frequency
 * approximately equal to the given bpm.
 */
void update_bpm() {
	if (bpm < HARD_MIN_BPM) {
		/* Counter can't do less than this amount of BPM. Argument values are limited elsewhere,
		 * but saturating the counter is a nicer than just letting it overflow.
		 */
		tock_period_floor = TIMER1_HIGHEST_COUNT;
		tock_period_remainder = 0;
	} else {
		/* See metronome.h for calculation */
		tock_period_floor = TOCK_PERIOD_FOR_1_BPM / bpm;
		tock_period_remainder = TOCK_PERIOD_FOR_1_BPM % bpm;

		// add 1 if it reduces integer division error
		// -> we can do better by dynamically correcting the period
		//if (tock_period_remainder >= bpm/2) {
		//	tock_period++;
		//}
	}

	/*
	 * Reset count of tocks modulo bpm, since it's used for frequency
	 * correction and we have a new bpm. But we keep the other tock counts
	 * the same, so that the position in the measure/beat is maintained.
	 */
	tock_num_modulo_bpm = 0;
	uint16_t new_OCR1A = calc_OCR1A();

    uint8_t old_SREG = SREG;
    cli();

    OCR1A = new_OCR1A;
    /* make sure that we don't miss a match, e.g. in the following situation
     ******|********************>                  |
           ^                    ^                  ^
        new max      current counter value          old_max
     * NB if the current counter value is close enough to old_max,
     * it might match by the time the comparison finishes, so do that
     * comparison second
     * NB2 I think this can also happen when increasing the BPM somehow, so we'll
     * check every time;
     */
    if (tock_period_floor <= TCNT1) {
        // trigger a match on next count
        TCNT1 = new_OCR1A - 1;
    }
    SREG = old_SREG;
}

static void metronome_tock() {
    if (tock_num_modulo_beat == 0) {
        // next beat
        metronome_beat();
        tick_num = 0;
    } else {
        if (tock_num_modulo_ticks == 0) {
            // metronome tick
            tick_num++;
            tone_gen_start(BEEP_FREQ_TICK);
        } else if (tock_num_modulo_ticks == BEEP_LENGTH_TOCKS) {
            // turn off current speaker tone
            tone_gen_stop();
            if (tick_num == 0 && (beat_num == 1 || beats_per_measure == 1)) {
                // just passed start of measure
                led_off();
            }
        }
    }

    /* Tock count calculations */

    tock_num_modulo_beat++;
    tock_num_modulo_ticks++;
    tock_num_modulo_bpm++;

    if (tock_num_modulo_bpm >= bpm) {
        tock_num_modulo_bpm = 0;
    }

    /* Normally for a given tock_period, we set OCR1A to 1 less than this number,
     * to give the desired reset frequency. However, if tock_period remainder
     * is nonzero, it means we would be (ever so) slightly too fast if we
     * always reset the counter every tock_period counts.
     * We can correct for this error by setting the timer to go for 1 extra
     * count on the first 'tock_period_remainder' times in every 'bpm' tocks.
     */
    // only do the check if there's actually a nonzero error/remainder
    if (tock_period_remainder >= 0) {
        // Check if we reached the switch-over threshold
        if (tock_num_modulo_bpm == tock_period_remainder) {
            /* TODO this is poor information hiding; decision logic should
             * be within calc_OCR1A function
             */
            OCR1A = calc_OCR1A();
        }
    }

    if (tock_num_modulo_beat >= TOCKS_PER_BEAT) {
        tock_num_modulo_beat = 0;
        tock_num_modulo_ticks = 0;
    } else if (tock_num_modulo_ticks >= tocks_for_ticks[ticks_per_beat]) {
        tock_num_modulo_ticks = 0;
    }
}


ISR(TIMER0_COMPA_vect) {
    muxed_7seg_timer_high_callback();
}

ISR(TIMER0_COMPB_vect) {
    muxed_7seg_timer_low_callback();
}

ISR(TIMER1_COMPA_vect) {
    metronome_tock();
}

/*
 * Setup switches as input pullup
 */
void input_setup() {
    // set 0 to configure as input
    bitClear(INPUT_DDR, SWITCHC);
    bitClear(INPUT_DDR, SWITCHU);
    bitClear(INPUT_DDR, SWITCHD);
    // write a 1 to set as pullup
    bitSet(INPUT_PORT, SWITCHC);
    bitSet(INPUT_PORT, SWITCHU);
    bitSet(INPUT_PORT, SWITCHD);
}


void configure_registers() {
	uint8_t oldSREG = SREG;
	cli();

	input_setup();
	led_setup();

	/* Disable unused peripherals */
	ADCSRA = 0; // ADC
	ADCSRB = 0; // analogue comparator
	/* remove clock signal to peripherals */
	PRR = 0;
	power_adc_disable();
	power_usart0_disable();
	// remove this for debugWIRE work
	power_spi_disable();
	power_twi_disable();


	SREG = oldSREG;
}


void setup() {
	configure_registers();

	bpm = 120;
	beats_per_measure = 4;
	ticks_per_beat = 1;
	beat_num = 0;
	tick_num = 0;
	tock_num_modulo_beat = 0;
	tock_num_modulo_ticks = 0;

	timer0_setup(16, 128);
	timer0_hold_reset();
	muxed_7seg_init();
	timer1_init();

	tone_gen_init();

    update_bpm();
	display_bpm();

	timer0_start();

	// initial tock
	metronome_tock();
}

void loop() {
	// check for change in BPM
	if (pressed(SWITCHU)) {
		if (pressed(SWITCHC)) {
			process_input(SWITCHU, &increment_ticks, TICKS_INCREMENT_REPEAT_RATE);
		} else {
			process_input(SWITCHU, &increment_bpm, BPM_INCREMENT_REPEAT_RATE);
		}
	} else if (pressed(SWITCHD)) {
		if (pressed(SWITCHC)) {
			process_input(SWITCHD, &increment_beats, TICKS_INCREMENT_REPEAT_RATE);
		} else {
			process_input(SWITCHD, &decrement_bpm, BPM_INCREMENT_REPEAT_RATE);
		}
	} else if (pressed(SWITCHC)) {
		display_timesig();
	} else {
        if (!displaying_bpm) {
            display_bpm();
        }
        delay(20);
    }

}

int main() {
	setup();
	for (;;) {
		loop();
	}
	return 0;
}


