//
// Created by max on 11/20/18.
//

#include "Metronome.h"
// kludge to get stuff to work together
#include "muxed_7seg.h"
#include "timers.h"
#include "tone_gen.h"
#include "byte_ops.h"
#include "pindefs.h"
#include "millis.h"
#include "SoftTimer.h"

#include <util/delay.h>
#include <avr/io.h>
// for the system clock prescale stuff
#include <avr/power.h>
#include <avr/interrupt.h>

static Metronome m;
static SoftTimer tickSoundTimer;
static bool displaying_bpm = false;

// At 16MHz / 64x prescaler, each tick of the soft timer takes 256*8 us,
// or 2.048 ms (see millis.cpp).
// So for a ~ 80ms countdown, we want to set a count of 40
inline static void setTickSoundTimer() {
    tickSoundTimer.setCount(30);
}

/*
 * Utility functions
 */

// negates value, since pins are configured as INPUT_PULLUP
inline static bool pressed(uint8_t input_pin) {
    return !bitRead(INPUT_REGISTER, input_pin);
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

inline static void still_alive() {
    constexpr int d = 100;
    for (int i = 0; i < 3; ++i) {
        led_on();
        _delay_ms(d);
        led_off();
        _delay_ms(d);

    }
}
inline static void still_alive2() {
    constexpr int d = 30;
    for (int i = 0; i < 3; ++i) {
        led_on();
        _delay_ms(d);
        led_off();
        _delay_ms(d);

    }
}

/**
 * Displays the BPM on the 7 segment displays
 */
inline static void display_bpm(uint8_t bpm) {
    muxed_7seg_show_number((int)bpm, false);
    displaying_bpm = true;
}

static void displaySubdivisions(uint8_t subdivision) {
    muxed_7seg_set_digit(2, 'd', WITH_DOT);
    muxed_7seg_set_digit(1, ' ', WITHOUT_DOT);
    muxed_7seg_set_digit(0, '0' + (char)(subdivision), WITHOUT_DOT);
    displaying_bpm = false;
}

static void displayMeasureLength(uint8_t measureLength) {
    muxed_7seg_set_digit(2, 'b', WITH_DOT);
    muxed_7seg_set_digit(1, '0' + (char)(measureLength / 10), WITHOUT_DOT);
    muxed_7seg_set_digit(0, '0' + (char)(measureLength % 10), WITHOUT_DOT);
    displaying_bpm = false;
}

/*
 * More meaty section
 */

/**
 * Calls a function when when a button press is detected
 * implements ability to hold down a button to automatically repeat the action
 * at the desired repeat rate (in ms)
 */
static void do_button_action_repeatable(uint8_t input_pin, void (*action)(), uint8_t repeat_rate) {
    action();
    // pause to allow single stepping
    const long current_time = millis();
    while (pressed(input_pin) && millis() - current_time < INCREMENT_REPEAT_DELAY) {
        delay(10);
    }
    // then repeat action at repeat rate
    while (pressed(input_pin)) {
        action();
        delay(repeat_rate);
    }
}

static void onBpmChange(uint8_t bpm) {
    display_bpm(bpm);
}

static void onMeasureLengthChange(uint8_t measureLength) {
    displayMeasureLength(measureLength);

}
static void onBeatSubdivisionChange(uint8_t subdivision) {
    displaySubdivisions(subdivision);
}

static void onBeat(uint8_t beat_num, uint8_t beats_per_measure) {
    if (beat_num == 0 && beats_per_measure > 0) {
        constexpr auto bar_tone = tone_gen_makeConfig(BEEP_FREQ_MEASURE);
        tone_gen_start(bar_tone);
        setTickSoundTimer();
        led_on();
    } else {
        // next beat
        constexpr auto beat_tone = tone_gen_makeConfig(BEEP_FREQ_BEAT);
        tone_gen_start(beat_tone);
        setTickSoundTimer();
    }
}

static void onTick(uint8_t tick_num, uint8_t ticks_per_beat) {
    if (tick_num != 1) {
        constexpr auto tick_tone = tone_gen_makeConfig(BEEP_FREQ_TICK);
        tone_gen_start(tick_tone);
        setTickSoundTimer();
    }
}

// used to turn off LED and tone after a beat or tick
static void postTickCallback() {
    tone_gen_stop();
    led_off();
}

static void incrementBpm() {
    m.incrementBpm(1_u8);
}

static void decrementBpm() {
    m.incrementBpm(0_u8 - 1_u8); // (uint8_t)-1
}

static void incrementMeasureLength() {
    m.incrementBeats();
}

static void incrementSubdivision() {
    m.incrementTicks();
}



ISR(TIMER0_COMPA_vect) {
    muxed_7seg_timer_high_callback();
}

ISR(TIMER0_COMPB_vect) {
    muxed_7seg_timer_low_callback();
}

ISR(TIMER1_COMPA_vect) {
    m.tock();
}

ISR(TIMER0_OVF_vect) {
    millis_timer0_callback();
    tickSoundTimer.tick();
}

/*
 * Setup switches as input pullup
 */
static void input_setup() {
    // just make whole of INPUT register input pullups
    INPUT_DDR = 0;
    INPUT_PORT = 0b01111111;
    // set 0 to configure as input
    //bitClear(INPUT_DDR, SWITCHC, SWITCHD, SWITCHU);
    // write a 1 to set as pullup
    //bitSet(INPUT_PORT, SWITCHC, SWITCHD, SWITCHU);
}


static void setup() {
    timer0_1_hold_reset();
    timer0_setup(16, 128);
    // timer 2
    tone_gen_init();
    muxed_7seg_init();

    input_setup();
    led_setup();

    /* Disable unused peripherals */
    ADCSRA = 0; // ADC
    ADCSRB = 0; // analogue comparator
    /* remove clock signal to peripherals */
    //PRR = 0;
    power_adc_disable();
    power_usart0_disable();
    // remove this for debugWIRE work
    power_spi_disable();
    power_twi_disable();


    // set up metronome
    m.setup();
    m.setBeatEventListener(onBeat);
    m.setTickEventListener(onTick);
    m.setBpmChangeCallback(onBpmChange);
    m.setBeatsChangeCallback(onMeasureLengthChange);
    m.setTicksChangeCallback(onBeatSubdivisionChange);

    tickSoundTimer.setAction(postTickCallback);
}

// poll inputs -> this should probably be done with interrupts
static void loop() {
    // check for change in BPM
    if (pressed(SWITCHU)) {
        if (pressed(SWITCHC)) {
            do_button_action_repeatable(SWITCHU, incrementSubdivision, TICKS_INCREMENT_REPEAT_RATE);
        } else {
            do_button_action_repeatable(SWITCHU, incrementBpm, BPM_INCREMENT_REPEAT_RATE);
        }
    } else if (pressed(SWITCHD)) {
        if (pressed(SWITCHC)) {
            do_button_action_repeatable(SWITCHD, incrementMeasureLength, TICKS_INCREMENT_REPEAT_RATE);
        } else {
            do_button_action_repeatable(SWITCHD, decrementBpm, BPM_INCREMENT_REPEAT_RATE);
        }
    } else if (pressed(SWITCHC)) {
        // TODO just rotate through displaying the different parameters;
    } else {
        if (!displaying_bpm) {
            display_bpm(m.getBpm());
        }
        delay(20);
    }

}

int main() {
    setup();
    timer0_1_start();

    m.start();

    // the magical command
    sei();

    for (;;) {
        loop();
    }
    return 0;
}
