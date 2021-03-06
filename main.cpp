//
// Created by max on 11/20/18.
//

#include "Metronome.h"
// kludge to get stuff to work together
#include "SevenSeg.h"
#include "timers.h"
#include "ToneGen.h"
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
static ToneGen t;
static SevenSeg sevenSeg;

/* All screens/display modes */
enum Screen {
    SCREEN_BLANK,
    SCREEN_BPM,
    SCREEN_MEASURE,
    SCREEN_SUBDIVIDE,
    NUM_SCREENS
};

static Screen nextScreen = SCREEN_BLANK;
static Screen currentScreen = SCREEN_BLANK;

static uint8_t buttonsState = 0;
static uint8_t lastButtonsState = 0;

// At 16MHz / 64x prescaler, each subBeat of the soft timer takes 256*8 us,
// or 2.048 ms (see millis.cpp).
inline static void setTickSoundTimer() {
    tickSoundTimer.setCount(30);
}

/*
 * Utility functions
 */

// call from PCINT interrupt handler
void onInputButtonsChange() {
    lastButtonsState = buttonsState;
    buttonsState = INPUT_REGISTER;
}

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
    auto intBpm = static_cast<int>(bpm);
    sevenSeg.showNumber(intBpm, false);
}

static void displaySubdivisions(uint8_t subdivision) {
    sevenSeg.setDigit(2, 'd', WITH_DOT);
    sevenSeg.setDigit(1, ' ', WITHOUT_DOT);
    sevenSeg.setDigit(0, '0' + (char)(subdivision), WITHOUT_DOT);
}

static void displayMeasureLength(uint8_t measureLength) {
    sevenSeg.setDigit(2, 'b', WITH_DOT);
    sevenSeg.setDigit(1, '0' + (char)(measureLength / 10), WITHOUT_DOT);
    sevenSeg.setDigit(0, '0' + (char)(measureLength % 10), WITHOUT_DOT);
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
        _delay_ms(10);
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
        constexpr auto bar_tone = ToneGen::makeConfig(BEEP_FREQ_MEASURE);
        t.start(bar_tone);
        setTickSoundTimer();
        led_on();
    } else {
        // next beat
        constexpr auto beat_tone = ToneGen::makeConfig(BEEP_FREQ_BEAT);
        t.start(beat_tone);
        setTickSoundTimer();
    }
}

static void onTick(uint8_t tick_num, uint8_t ticks_per_beat) {
    // don't play a sound on the actual beat
    // TODO I don't know why this works, rather than a != 0 check
    if (tick_num != 0)
    {
        constexpr auto tick_tone = ToneGen::makeConfig(BEEP_FREQ_SUB);
        t.start(tick_tone);
        setTickSoundTimer();
    }
}

// used to turn off LED and tone after a beat or subBeat
static void postTickCallback() {
    t.stop();
    led_off();
}

static void incrementBpm() {
    m.incrementBpm(1);
}

static void decrementBpm() {
    m.incrementBpm(static_cast<uint8_t>(-1)); // (uint8_t)-1
}

static void setNextScreen(Screen s) {
    nextScreen = s;
}

static void incrementNextScreen() {
    int nextScreenIdx = currentScreen + 1;
    if (nextScreenIdx >= NUM_SCREENS) {
        nextScreenIdx = 0;
    }
    nextScreen = static_cast<Screen>(nextScreenIdx);

}

static void incrementMeasureLength() {
    m.incrementBeats(1);
}
static void decrementMeasureLength() {
    m.incrementBeats(static_cast<uint8_t>(-1));
}

static void incrementSubdivision() {
    m.incrementTicks(static_cast<uint8_t>(1));
}
static void decrementSubdivision() {
    m.incrementTicks(static_cast<uint8_t>(-1));
}

ISR(TIMER0_COMPA_vect) {
    sevenSeg.timerHighCallback();
}

ISR(TIMER0_COMPB_vect) {
    sevenSeg.timerLowCallback();
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
}

static void setup() {
    /* Disable unused peripherals */
    ADCSRA = 0; // ADC
    ADCSRB = 0; // analogue comparator
    /*
     * PRR
     * bit 7                                              bit0
     * PRTWI  PRTIM2 PRTIM0    -    PRTIM1 PRSPI  PRUSART PRADC
     */
    // disable ADC, TWI, SPI, USART
    // enable timer0-2, disable TWI, SPI, UART0, ADC
    PRR = 0b10000111;

    timer0_1_hold_reset();
    timer0_setup(16, 128);
    // timer 2
    t.setup();
    sevenSeg.setup();

    input_setup();
    led_setup();



    // set up metronome
    m.setup();
    m.setBeatEventListener(onBeat);
    m.setTickEventListener(onTick);
    m.setBpmChangeCallback(onBpmChange);
    m.setBeatsChangeCallback(onMeasureLengthChange);
    m.setTicksChangeCallback(onBeatSubdivisionChange);

    tickSoundTimer.setAction(postTickCallback);
}

static void updateScreen() {
    switch (nextScreen) {
        case SCREEN_BPM:
            display_bpm(m.getBpm());
            sevenSeg.displayOn();
            break;
        case SCREEN_MEASURE:
            displayMeasureLength(m.getMeasureLength());
            sevenSeg.displayOn();
            break;
        case SCREEN_SUBDIVIDE:
            displaySubdivisions(m.getBeatSubdivisions());
            sevenSeg.displayOn();
            break;
        case SCREEN_BLANK:
            sevenSeg.displayOff();
            break;
    }
    currentScreen = nextScreen;
}

// poll inputs -> this should probably be done with interrupts
static void loop() {
    if (pressed(SWITCHC)) {
        incrementNextScreen();
        updateScreen();
        while (pressed(SWITCHC));
        _delay_ms(20);
    } else if (pressed(SWITCHS)) {
        m.toggle();
        // wait until button unpressed
        while (pressed(SWITCHS));
        _delay_ms(20);
    } else {
        if (pressed(SWITCHU)) {
            switch (currentScreen) {
                case SCREEN_BPM:
                    do_button_action_repeatable(SWITCHU, incrementBpm, BPM_INCREMENT_REPEAT_RATE);
                    break;
                case SCREEN_MEASURE:
                    do_button_action_repeatable(SWITCHU, incrementMeasureLength, TICKS_INCREMENT_REPEAT_RATE);
                    break;
                case SCREEN_SUBDIVIDE:
                    do_button_action_repeatable(SWITCHU, incrementSubdivision, TICKS_INCREMENT_REPEAT_RATE);
                    break;
                default:
                    break;
            }
        } else if (pressed(SWITCHD)) {
            switch (currentScreen) {
                case SCREEN_BPM:
                    do_button_action_repeatable(SWITCHD, decrementBpm, BPM_INCREMENT_REPEAT_RATE);
                    break;
                case SCREEN_MEASURE:
                    do_button_action_repeatable(SWITCHD, decrementMeasureLength, TICKS_INCREMENT_REPEAT_RATE);
                    break;
                case SCREEN_SUBDIVIDE:
                    do_button_action_repeatable(SWITCHD, decrementSubdivision, TICKS_INCREMENT_REPEAT_RATE);
                    break;
                default:
                    break;
            }
        }
    }
}



int main() {
    setup();
    timer0_1_start();
    m.start();
    sevenSeg.displayOn();

    // the magical command
    sei();

    // startup procedure
    nextScreen = SCREEN_BPM;
    updateScreen();

    for (;;) {
        loop();
    }
    return 0;
}
