#include "apu.h"
#include "cpu.h"
#include "types.h"
#include <math.h>
#include <pthread.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SAMPLE_RATE 44100.f
#define CPU_FREQ 1789773.f

static apu_t *this = NULL;

static uint8_t length_timer_lookup[] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static uint16_t noise_timer_lookup[] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

static uint16_t dpcm_timer_lookup[] = {428, 380, 340, 320, 286, 254, 226, 214,
                                       190, 160, 142, 128, 106, 84,  72,  54};

float triangle_wave(float x) {
    while (x >= 2 * M_PI)
        x -= 2 * M_PI;
    if (x < M_PI)
        return x / M_PI;
    return -((x - M_PI) / M_PI);
}

float square_wave(float x, duty_cycle c) {
    while (x >= 2 * M_PI)
        x -= 2 * M_PI;
    static float duty_cycle_lookup[4][8] = {
        {-1, 1, -1, -1, -1, -1, -1, -1},
        {-1, 1, 1, -1, -1, -1, -1, -1},
        {-1, 1, 1, 1, 1, -1, -1, -1},
        {1, -1, -1, 1, 1, 1, 1, 1},
    };

    return duty_cycle_lookup[c][(uint8_t)floor(x / M_PI_4)];
}

void noise_callback(void *buffer, uint32_t count) {
    static float pending_shifts = 0;
    int16_t *out = (int16_t *)buffer;
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (((this->channel4.lfsr & 1) ? 0 : 32000) *
                  ((this->channel4.length_counter == 0) ? 0 : 1) *
                  (this->channel4.volume_level / 15.f) * this->volume);
        pending_shifts +=
            (CPU_FREQ / this->channel4.timer_period) / SAMPLE_RATE;
        while (pending_shifts > 0) {
            bool feedback =
                (this->channel4.lfsr & 1) ^
                ((this->channel4.lfsr >> (this->channel4.mode ? 6 : 1)) & 1);
            this->channel4.lfsr >>= 1;
            this->channel4.lfsr |= feedback << 14;
            pending_shifts--;
        }
    }
}

void triangle_callback(void *buffer, uint32_t count) {
    static float sine_idx = 0.f;
    float incr = this->channel3.frequency / SAMPLE_RATE;
    int16_t *out = (int16_t *)buffer;

    for (uint32_t i = 0; i < count; i++) {
        out[i] = (int16_t)(32000.f * triangle_wave(2 * M_PI * sine_idx) *
                           ((this->channel3.length_counter == 0) ? 0 : 1) *
                           ((this->channel3.linear_counter == 0) ? 0 : 1) *
                           this->volume);
        sine_idx += incr;
        if (sine_idx > 1.f)
            sine_idx -= 1.f;
    }
}

void square1_callback(void *buffer, uint32_t count) {
    static float sine_idx = 0.f;
    static float pending_envelope_ticks = 0.f;
    float incr = this->channel1.frequency / SAMPLE_RATE;
    int16_t *out = (int16_t *)buffer;

    for (uint32_t i = 0; i < count; i++) {
        out[i] =
            (int16_t)(32000.f *
                      square_wave(2 * M_PI * sine_idx, this->channel1.duty) *
                      (this->channel1.enable ? 1 : 0) *
                      (this->channel1.constant_volume
                           ? (this->channel1.envelope_level / 15.f)
                           : (this->channel1.envelope_decay_counter / 15.f)) *
                      ((this->channel1.timer < 8 ||
                        this->channel1.timer > 0x7ff)
                           ? 0
                           : 1) *
                      ((this->channel1.length_counter == 0) ? 0 : 1) *
                      this->volume);
        sine_idx += incr;
        if (sine_idx > 1.f)
            sine_idx -= 1.f;
        pending_envelope_ticks += (1 / SAMPLE_RATE) / count;
        while(pending_envelope_ticks > (this->channel1.envelope_level + 1) / 240.f) {
            if(this->channel1.envelope_decay_counter == 0) {
                if(this->channel1.loop) this->channel1.envelope_decay_counter = 0xf;
            } else this->channel1.envelope_decay_counter--;
            pending_envelope_ticks -= (this->channel1.envelope_level + 1) / 240.f;
        }
    }
}

void square2_callback(void *buffer, uint32_t count) {
    static float sine_idx = 0.f;
    static float pending_envelope_ticks = 0.f;
    float incr = this->channel2.frequency / SAMPLE_RATE;
    int16_t *out = (int16_t *)buffer;

    for (uint32_t i = 0; i < count; i++) {
        out[i] =
            (int16_t)(32000.f *
                      square_wave(2 * M_PI * sine_idx, this->channel2.duty) *
                      (this->channel2.enable ? 1 : 0) *
                      (this->channel2.constant_volume
                           ? (this->channel2.envelope_level / 15.f)
                           : (this->channel2.envelope_decay_counter / 15.f)) *
                      ((this->channel2.timer < 8 ||
                        this->channel2.timer > 0x7ff)
                           ? 0
                           : 1) *
                      ((this->channel2.length_counter == 0) ? 0 : 1) *
                      this->volume);
        sine_idx += incr;
        if (sine_idx > 1.f)
            sine_idx -= 1.f;
        pending_envelope_ticks += (1 / SAMPLE_RATE) / count;
        while(pending_envelope_ticks > (this->channel2.envelope_level + 1) / 240.f) {
            if(this->channel2.envelope_decay_counter == 0) {
                if(this->channel2.loop) this->channel2.envelope_decay_counter = 0xf;
            } else this->channel2.envelope_decay_counter--;
            pending_envelope_ticks -= (this->channel2.envelope_level + 1) / 240.f;
        }
    }
}

apu_t *get_apu_handle(void) { return this; }

void *timer_cb(void *_) {
    uint32_t counter = 1;
    uint8_t channel1_divider_period = 1;
    uint8_t channel2_divider_period = 1;
    while (1) {
        usleep(this->long_sequence ? 5208 : 4166);
        if (!this->channel1.loop && this->channel1.length_counter > 0) {
            this->channel1.length_counter--;
        }
        if (!this->channel2.loop && this->channel2.length_counter > 0) {
            this->channel2.length_counter--;
        }
        if (!this->channel3.length_counter_halt &&
            this->channel3.length_counter > 0) {
            this->channel3.length_counter--;
        }
        if (counter % 2 == 0) {
            if (!this->channel4.loop && this->channel4.length_counter > 0) {
                this->channel4.length_counter--;
            }

            if (--channel1_divider_period == 0) {
                channel1_divider_period = this->channel1.sweep_speed + 1;
                if (this->channel1.sweep_enable && this->channel1.timer >= 8 && this->channel1.sweep_shift_count != 0) {
                    int16_t offset = this->channel1.timer >>
                                     this->channel1.sweep_shift_count;
                    if (this->channel1.sweep_negate) {
                        this->channel1.timer -= offset + 1;
                    } else if (this->channel1.timer + offset < 0x800) {
                        this->channel1.timer += offset;
                    }
                    this->channel1.frequency =
                        CPU_FREQ / (16.f * (this->channel1.timer + 1));
                }
            }

            if (--channel2_divider_period == 0) {
                channel2_divider_period = this->channel2.sweep_speed + 1;
                if (this->channel2.sweep_enable && this->channel2.timer >= 8 && this->channel2.sweep_shift_count != 0) {
                    int16_t offset = this->channel2.timer >>
                                     this->channel2.sweep_shift_count;
                    if (this->channel2.sweep_negate) {
                        this->channel2.timer -= offset;
                    } else if (this->channel2.timer + offset < 0x800) {
                        this->channel2.timer += offset;
                    }
                    this->channel2.frequency =
                        CPU_FREQ / (16.f * (this->channel2.timer + 1));
                }
            }
        }

        if (counter % 4 == 0 && !this->long_sequence &&
            !this->interrupt_inhibit) {
            get_cpu_handle()->irq = true;
        }

        if (this->channel3.linear_counter > 0)
            this->channel3.linear_counter--;

        counter++;
    }

    return NULL;
}

void apu_init(apu_t *apu) {
    this = apu;
    this->volume = 0.01;
    this->channel4.lfsr = 1;
    this->channel4.timer_period = noise_timer_lookup[0];
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    this->channel1.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(this->channel1.stream, square1_callback);
    PlayAudioStream(this->channel1.stream);
    this->channel2.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(this->channel2.stream, square2_callback);
    PlayAudioStream(this->channel2.stream);
    this->channel3.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(this->channel3.stream, triangle_callback);
    PlayAudioStream(this->channel3.stream);
    this->channel4.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(this->channel4.stream, noise_callback);
    PlayAudioStream(this->channel4.stream);
    this->channel5.stream = LoadAudioStream(SAMPLE_RATE, 16, 1);

    pthread_t apu_clock;
    pthread_create(&apu_clock, NULL, timer_cb, NULL);
}

uint8_t apu_status_read(void) {
    return ((this->channel1.length_counter > 0) << 0) |
           ((this->channel2.length_counter > 0) << 1) |
           ((this->channel3.length_counter > 0) << 2) |
           ((this->channel4.length_counter > 0) << 3);
}

void apu_status_write(uint8_t value) {
    this->channel1.enable = value & 0b1;
    this->channel2.enable = value & 0b10;
    if ((this->channel3.enable = (value & 0b100)) == false) {
        this->channel3.length_counter = 0;
    }
    this->channel4.enable = value & 0b1000;
    this->channel5.enable = value & 0b10000;
}

void pulse_1_config(uint8_t value) {
    this->channel1.duty = value >> 6;
    this->channel1.loop = (value & 0x20) != 0;
    this->channel1.constant_volume = (value & 0x10) != 0;
    this->channel1.envelope_level = value & 0xf;
}

void pulse_1_timer_low(uint8_t value) {
    this->channel1.timer &= 0xff00;
    this->channel1.timer |= value;
    this->channel1.frequency = CPU_FREQ / (16.f * (this->channel1.timer + 1));
}

void pulse_1_timer_high(uint8_t value) {
    this->channel1.timer &= 0xff;
    this->channel1.timer |= (value & 0x7) << 8;
    this->channel1.frequency = CPU_FREQ / (16.f * (this->channel1.timer + 1));
    if (this->channel1.enable) {
        this->channel1.length_counter = length_timer_lookup[value >> 3];
    }
    this->channel1.envelope_decay_counter = 0xf;
}

void pulse_1_sweep(uint8_t value) {
    this->channel1.sweep_enable = (value & 0x80) != 0;
    this->channel1.sweep_speed = (value >> 4) & 0x7;
    this->channel1.sweep_negate = (value & 0x8) != 0;
    this->channel1.sweep_shift_count = value & 0x7;
}

void pulse_2_config(uint8_t value) {
    this->channel2.duty = value >> 6;
    this->channel2.loop = (value & 0x20) != 0;
    this->channel2.constant_volume = (value & 0x10) != 0;
    this->channel2.envelope_level = value & 0xf;
}

void pulse_2_timer_low(uint8_t value) {
    this->channel2.timer &= 0xff00;
    this->channel2.timer |= value;
    this->channel2.frequency = CPU_FREQ / (16.f * (this->channel2.timer + 1));
}

void pulse_2_timer_high(uint8_t value) {
    this->channel2.timer &= 0xff;
    this->channel2.timer |= (value & 0x7) << 8;
    this->channel2.frequency = CPU_FREQ / (16.f * (this->channel2.timer + 1));
    if (this->channel2.enable) {
        this->channel2.length_counter = length_timer_lookup[value >> 3];
    }
    this->channel2.envelope_decay_counter = 0xf;
}

void pulse_2_sweep(uint8_t value) {
    this->channel2.sweep_enable = (value & 0x80) != 0;
    this->channel2.sweep_speed = (value >> 4) & 0x7;
    this->channel2.sweep_negate = (value & 0x8) != 0;
    this->channel2.sweep_shift_count = value & 0x7;
}

void triangle_linear_counter(uint8_t value) {
    this->channel3.length_counter_halt = (value & 0x80) != 0;
    this->channel3.linear_counter = value & 0x7f;
}

void triangle_timer_low(uint8_t value) {
    this->channel3.timer &= 0xff00;
    this->channel3.timer |= value;
    this->channel3.frequency = CPU_FREQ / (32.f * (this->channel3.timer + 1));
}

void triangle_timer_high(uint8_t value) {
    this->channel3.timer &= 0xff;
    this->channel3.timer |= (value & 0x7) << 8;
    this->channel3.frequency = CPU_FREQ / (32.f * (this->channel3.timer + 1));
    if (this->channel3.enable) {
        this->channel3.length_counter = length_timer_lookup[value >> 3];
    }
}

void noise_config(uint8_t value) {
    this->channel4.loop = (value & 0x20) != 0;
    this->channel4.constant_volume = (value & 0x10) != 0;
    if (this->channel4.constant_volume)
        this->channel4.volume_level = value & 0xf;
    else
        this->channel4.volume_level_speed = value & 0xf;
}

void noise_period(uint8_t value) {
    this->channel4.mode = (value & 0x80) != 0;
    this->channel4.timer_period = noise_timer_lookup[value & 0xf];
}

void noise_length_counter(uint8_t value) {
    if (this->channel4.enable) {
        this->channel4.length_counter = length_timer_lookup[value >> 3];
    }
}

void frame_counter(uint8_t value) {
    this->long_sequence = (value & 0x80) != 0;
    this->interrupt_inhibit = (value & 0x40) != 0;
}

void dpcm_freq(uint8_t value) {
    this->channel5.irq_enable = value & 0x80;
    this->channel5.loop = value & 0x40;
    this->channel5.rate_index = value & 0xf;
}

void dpcm_sample_addr(uint8_t value) {
    this->channel5.current_address = 0xc000 + value * 64;
}

void dpcm_sample_length(uint8_t value) {
    this->channel5.sample_length = 16 * value + 1;
}

void dpcm_direct_load(uint8_t value) {
    this->channel5.buffer = value & 0x7f;
    this->channel5.remaining_bits = 8;
}
