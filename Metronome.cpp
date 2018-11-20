#include "Metronome.h"
#include "defines.h"


#include <avr/interrupt.h>
#include <avr/io.h>

/*
 * Adds the given increment to the specified counter, ensuring that the count
 * remains within the interval [low, high].
 * Have to ensure that the counter does not overflow due to the increment
 * before applying the [low, high] limits, and that howMuch
 */
static inline uint8_t change(uint8_t what, uint8_t howMuch, uint8_t low, uint8_t high) {
    uint8_t range = high - low + 1_u8;
    uint8_t changed = what + howMuch;
    // put back into range
    if (changed < low) {
        changed += range;
    } else if (changed > high) {
        changed -= range;
    }

    return changed;
}

/* increases or decreases the stored value for bpm by 1, keeping it within range,
 * displays it on the 7 segment displays, and also adjusts the Timer1 compare register
 * to trigger the TIM1_COMPA interrupt at the corresponding frequency.
 */
void Metronome::incrementBpm(uint8_t increment) {
    auto newValue = change(bpm, increment, SOFT_MIN_BPM, SOFT_MAX_BPM);
    setBpm(newValue);
}

void Metronome::setBpm(uint8_t newValue) {
    bpm = newValue;
    update_timer();
    onBpmChanged(newValue);
}

void Metronome::setMeasureLength(uint8_t newValue) {
    beats_per_measure = newValue;
    onBeatsChanged(newValue);
}

void Metronome::setBeatDivision(uint8_t newValue) {
    ticks_per_beat = newValue;
    onTicksChanged(newValue);

}
void Metronome::incrementBeats() {
    auto new_value = change(beats_per_measure, 1_u8, MIN_BEATS_PER_MEASURE, MAX_BEATS_PER_MEASURE);
    setMeasureLength(new_value);
}

void Metronome::incrementTicks() {
    auto new_value = change(ticks_per_beat, 1_u8, MIN_TICKS_PER_BEAT, MAX_TICKS_PER_BEAT);
    setBeatDivision(new_value);
}

void Metronome::start() {
    // enable 8x I/O clock prescaler for timer1
    bitSet(TCCR1B, CS11);
}

void Metronome::reset() {
    auto sreg = SREG;

    cli();
    beat_num = 0;
    tick_num = 1;
    TCNT1 = 0;

    SREG = sreg;
}

void Metronome::pause() {
    // disconnect prescaler
    bitClear(TCCR1B, CS11);

}

void Metronome::setBeatEventListener(const Metronome::twoParamCallback &f) {
    onBeat = f;
}
void Metronome::setTickEventListener(const Metronome::twoParamCallback &f) {
    onTick = f;
}
void Metronome::setBpmChangeCallback(const Metronome::oneParamCallback &f) {
    onBpmChanged = f;
}
void Metronome::setBeatsChangeCallback(const Metronome::oneParamCallback &f) {
    onBeatsChanged = f;
}
void Metronome::setTicksChangeCallback(const Metronome::oneParamCallback &f) {
    onTicksChanged = f;
}

void Metronome::timerSetup() {
// call timer0_1_hold_reset() before this and timer_0_1_start() after
    auto sreg = SREG;
    cli();

    // configure control registers as follows:
    TCCR1A = 0;
    TCCR1B = 0;
    // disconnect both output pins (9 and 10) for timer 1
    // COM1A[1:0], COM1B[1:0] = 0
    // set waveform generation mode to clear timer on compare match
    bitSet(TCCR1B, WGM12);
    // enable interrupt for Timer1A Compare match
    bitSet(TIMSK1, OCIE1A);

    SREG = sreg;
}

void Metronome::setup() {
    // don't use the set functions since they do callbacks
    timerSetup();

    beats_per_measure = 4;
    ticks_per_beat = 1;
    bpm = 105;
    update_timer();

}

uint8_t Metronome::getBpm() const {
    return bpm;
}

uint8_t Metronome::getMeasureLength() const {
    return beats_per_measure;
}

uint8_t Metronome::getBeatSubdivisions() const {
    return ticks_per_beat;
}

/*
 * OCR1A should normally be 1 less than the desired count period,
 * since it resets to 0.
 */

/* Normally for a given tock_period, we set OCR1A to 1 less than tock_period_floor
 * to give the desired reset frequency. However, if tock_period remainder
 * is nonzero, it means we would be (ever so) slightly too fast if we
 * always reset the counter every tock_period counts.
 * We can correct for this error by setting the timer to go for 1 extra
 * count on the first 'tock_period_remainder' times in every 'bpm' tocks.
 */

inline uint16_t Metronome::calc_timer_count() {
    if (tock_num_modulo_bpm < tock_period_remainder) {
        // if there is a rounding down error in tock_period, we should
        // initially count by 1 extra, then go back to tock_period_floor - 1.
        return tock_period_floor; // - 1 + 1;
    } else {
        // Have 'overcounted' enough / tock_period had no remainder
        return tock_period_floor - 1_u16;
    }
}

inline void Metronome::timer_count_dynamic_adjust() {
    // Check if we reached the switch-over threshold, otherwise it's the same
    // TODO have instance variable to record which side of the threshold we were on
    if (tock_num_modulo_bpm == 0 || tock_num_modulo_bpm == tock_period_remainder) {
        auto new_ocr1A = calc_timer_count();

        auto sreg = SREG;
        cli();
        OCR1A = new_ocr1A;
        SREG = sreg;
    }
}


/* writes the appropriate value to OCR1A so that timer 1 resets with frequency
 * approximately equal to the given bpm.
 */
void Metronome::update_timer() {
    if (bpm < HARD_MIN_BPM) {
        // BPM is too slow to keep a full count, so just maximise w/o overflow
        tock_period_floor = TIMER1_HIGHEST_COUNT;
        tock_period_remainder = 0;
    } else {
        tock_period_floor = static_cast<uint16_t>(TOCK_PERIOD_FOR_1_BPM / bpm);
        tock_period_remainder = static_cast<uint8_t>(TOCK_PERIOD_FOR_1_BPM % bpm);
    }

    /*
     * Reset count of tocks modulo bpm, since it's used for frequency
     * correction and we have a new bpm. But we keep the other tock counts
     * the same, so that the position in the measure/beat is maintained.
     */
    tock_num_modulo_bpm = 0;
    uint16_t new_OCR1A = calc_timer_count();

    uint8_t old_SREG = SREG;
    cli();

    OCR1A = new_OCR1A;
    /* make sure that we don't miss a match, e.g. in the following situation
     ******|********************>                  |
           ^                    ^                  ^
        new max      current counter value          old_max
     * NB if the current counter value is close enough to old_max, it might
     * match by the time the comparison finishes, so do that comparison second
     * NB2 I think this can also happen when increasing the BPM somehow, so we'll
     * check every time;
     */
    if (new_OCR1A <= TCNT1) {
        // trigger a match on next count
        TCNT1 = new_OCR1A - 1_u16;
    }
    SREG = old_SREG;
}

inline void Metronome::beat() {
    onBeat(beat_num, beats_per_measure);
    beat_num++;
    // need >= check (not just ==) in case beats_per_measure = 0
    if (beat_num >= beats_per_measure) {
        beat_num = 0;
    }
}

inline void Metronome::tick() {
    onTick(tick_num, ticks_per_beat);
    tick_num++;
    if (tick_num > ticks_per_beat) {
        tick_num = 1;
    }
}

/* how many tocks happen before we play a subdivided beat 'tick'
 * 1 tick  per beat -> 60 tocks per tick
 * 2 ticks per beat -> 30 tocks per tick
 * 3 ticks per beat -> 20 tocks per tick
 * 4 ticks per beat -> 15 tocks per tick
 * 5 ticks per beat -> 12 tocks per tick
 * 6 ticks per beat -> 10 tocks per tick
 * Example: ticks_per_beat = 3
 *              |*********|*********|*********|*********|*********|*********|
 * tocks>       0         10        20        30        40        50        60->0
 * ticks/beats> B                   T                   T                   B
 * Example: ticks_per_beat = 4
 *              |**************|**************|**************|**************|
 * tocks>       0              15             30             45             60->0
 * ticks/beats> B              T              T              T              B
 */
// 1- indexed, first entry is filler
static const uint8_t tocks_for_ticks[] {0, 60, 30, 20, 15, 12, 10};

void Metronome::tock() {
    /* Metronome event checks */
    if (tock_num_modulo_ticks == 0) {
        tick();
    }
    if (tock_num_modulo_beat == 0) {
        beat();
    }

    tock_num_modulo_beat++;
    tock_num_modulo_ticks++;

    // check if we've reached the next tick or beat
    if (tock_num_modulo_ticks >= tocks_for_ticks[ticks_per_beat]) {
        tock_num_modulo_ticks = 0;
    }
    if (tock_num_modulo_beat >= TOCKS_PER_BEAT) {
        tock_num_modulo_beat = 0;
        tock_num_modulo_ticks = 0;
    }

    /* BPM correction calculations */

    /* If there's a nonzero error/remainder when we calculated the timer count
     * for a given BPM, then we may need to do dynamic adjustement of the
     * count, in order to make it more accurate
     */
    if (tock_period_remainder > 0) {
        timer_count_dynamic_adjust();
    }

    tock_num_modulo_bpm++;

    if (tock_num_modulo_bpm >= bpm) {
        tock_num_modulo_bpm = 0;
    }
}
