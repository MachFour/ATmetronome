/**
 * metronome.h: defines, constants and function prototypes for metronome.ino
 */
#ifndef METRONOME_H
#define METRONOME_H

#include <avr/io.h>


// beep sound parameters
constexpr unsigned int BEEP_FREQ_MEASURE = 554u << 1u;
constexpr auto BEEP_FREQ_BEAT = 440u << 1u;
constexpr auto BEEP_FREQ_TICK = 293u << 1u;
constexpr auto BEEP_LENGTH_TOCKS = 4;

// parameters for button controls, all in ms
constexpr auto BPM_INCREMENT_REPEAT_RATE = 10;
constexpr auto TICKS_INCREMENT_REPEAT_RATE = 100;
constexpr auto INCREMENT_REPEAT_DELAY = 300;

#define WITHOUT_DOT false
#define WITH_DOT true

/**
 * Terms:
 * beat - an event that occurs with frequency (in Hz)
 *        approximately equal to the user-entered bpm divided by 60.
 *        This is the main 'event' of the metronome, and is always audible.
 * tock - a tock happens every 1/TOCKS_PER_BEAT (e.g. 1/60) of a beat duration.
 *        This allows subdivisions of the beat based on multiples of tocks,
 *        which is easy to achieve with counters.
 *        Tocks themselves are not exposed to the user.
 * tick - a user-adjustable subdivision of the beat. Happens every
 *        (TOCKS_PER_BEAT/ticks_per_beat)th tock (exactly divisible, by design)
 *        The metronome plays an audible sound on every tock, with a different
 *        pitch to the main beat.
 * measure - musical concept which groups beats into groups / bars.
 *        The first beat of every measure is accented unless beats_per_measure is 0
 */

/* This value is determined by the value in TCCR1B,
 * and is set during timer_init()
 */
#define TIMER1_PRESCALE 8

/* From datasheet on CTC mode:
 * fOCR1A = fclk_io /(2*N*(1+OCR1A))
 * where fOCR1A is the frequency of a PWM wave generated by toggling OC1A
 * on every Timer1 match with OCR1A, fclk_io is the speed of the prescaler,
 * which in this case is equal to F_CPU, N is the prescale value set for timer 1,
 * and OCR1A is the value for timer1 at which it resets.
 * Here, we have N=8, fclk_io = 8000000, and f_tock = fOCR1A*2, since we tock on
 * every reset of the timer, whereas one period of a PWM wave lasts two resets.
 * Thus,
 * f_beat = f_tock/60
 *        = fOCR1A/30
 *        = 8000000/(16*30*(1+OCR1A
 *        = 50000/(3*(1 + OCR1A)).
 * For 1BPM, f_beat = 1/60, so 1 + OCR1A = 50000*60/3 = 1000000
 * thus to get an arbitrary integer BPM, we need to divide 1000000 by the BPM,
 * correct for any integer division error, and then subtract 1
 */

static constexpr uint32_t TOCK_PERIOD_FOR_1_BPM = 1000000;

static constexpr uint16_t TIMER1_HIGHEST_COUNT = 65535;

static constexpr uint8_t TOCKS_PER_BEAT = 60;

#define HARD_MIN_BPM 16 // ceil(TIMER1_COUNT_FOR_1BPM/(TIMER1_HIGHEST_COUNT + 1))
#define SOFT_MIN_BPM 30
// will overflow in uint8_t otherwise
#define SOFT_MAX_BPM 254

#define MIN_BEATS_PER_MEASURE 0
#define MAX_BEATS_PER_MEASURE 16
#define MIN_TICKS_PER_BEAT 1
#define MAX_TICKS_PER_BEAT 6

/* Global variables */

static bool displaying_bpm;

/* How fast a 'crotchet' is in beats per minute */
static uint8_t bpm;
/* A measure is like a bar, and the first beat of each measure is accented.
 * Has no other effect other than 'accent the nth crotchet'
 * If this is set to zero then no accents are played.
 */
static uint8_t beats_per_measure;
/* Each beat is evenly subdivided into 'ticks'
 * Used to play quavers, semiquavers, triplets etc.
 * Patten of accents can be programmed using the DIP switches.
 */
static uint8_t ticks_per_beat;

/* where we are in the measure */
static uint8_t beat_num;
static uint8_t tick_num;

// Counts once from 0 to TOCKS_PER_BEAT - 1 every beat
static uint8_t tock_num_modulo_beat;

/* These variables are used to control BPM (actually, tock) duration
 * via timer 1 resets. In order to remove error from integer division,
 * the timer1 reset count is adaptively adjusted, so that the reset
 * can average out to be a rational number.
 * For example, a count period of 100.5 can be achieved by counting to 100
 * half the time and 101 the other half of the time. In order to do this,
 * we need the following variables:
 * tock_period_floor
 *      The period corresponding to the desired BPM.
 *      A tock period of 1 denotes the time taken per increment of timer1
 * tock_period_remainder
 *      Stores the remainder of TOCK_PERIOD_FOR_1BPM / bpm
 * tock_num_modulo_bpm
 *      Counts from 0 to bpm - 1 and then is reset to 0. No relation to beats or ticks.
 *      The correct way to achieve rational tock periods using integer
 *      timer counts is to have a period of:
 *      tock_period_floor for (tock_period_remainder/bpm) of the time, and
 *      tock_period_floor+1 for (1 - tock_period_remainder/bpm) of the time
 *      The easiest way to achieve this is to set the actual timer count
 *      to tock_period_floor+1 initially, and then reduce it by 1 when
 *      tock_num_modulo_bpm reaches tock_period_remainder.
 */
static uint16_t tock_period_floor;
static uint8_t tock_period_remainder; // less than BPM
static uint8_t tock_num_modulo_bpm; // also less than BPM

// Counts from 0 to tocks_for_ticks[ticks_per_beat] - 1 several times per beat
// (The number of times this happens is precisely ticks_per_beat)
static uint8_t tock_num_modulo_ticks;

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
static uint8_t tocks_for_ticks[7] {0, 60, 30, 20, 15, 12, 10};

/* function definitions */
void timer_init();
void timer_start();
void update_bpm();
static void metronome_tock();
static void metronome_beat();
void increment_ticks();

#endif
