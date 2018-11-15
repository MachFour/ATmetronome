#ifndef TONE_GEN_H
#define TONE_GEN_H

#include <stdint.h>

void tone_gen_init();
void tone_gen_start(uint16_t frequency);
void tone_gen_stop();

#endif // TONE_GEN_H
