#ifndef APU_H
#define APU_H

#include "types.h"

void apu_init(apu_t *apu);

void apu_status(uint8_t value);

void pulse_1_config(uint8_t value);
void pulse_1_timer_low(uint8_t value);
void pulse_1_timer_high(uint8_t value);
void pulse_1_sweep(uint8_t value);

void pulse_2_config(uint8_t value);
void pulse_2_timer_low(uint8_t value);
void pulse_2_timer_high(uint8_t value);
void pulse_2_sweep(uint8_t value);

void triangle_linear_counter(uint8_t value);
void triangle_timer_low(uint8_t value);
void triangle_timer_high(uint8_t value);

void noise_config(uint8_t value);
void noise_period(uint8_t value);
void noise_length_counter(uint8_t value);

void dpcm_direct_load(uint8_t value);

void frame_counter(uint8_t value);

#endif
