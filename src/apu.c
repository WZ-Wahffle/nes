#include "apu.h"
#include "types.h"
#include <math.h>
#include <raylib.h>
#include <stdlib.h>

static apu_t *this = NULL;

static uint8_t length_timer_lookup[] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static uint16_t noise_timer_lookup[] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

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
        {0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 1, 1, 1, 1},
    };

    return duty_cycle_lookup[c][(uint8_t)floor(x / M_PI_4)];
}

void triangle_callback(void *buffer, uint32_t count) {
    static float sineIdx = 0.f;
    float incr = this->channel3.frequency / 44100.f;
    int16_t *out = (int16_t *)buffer;

    for (uint16_t i = 0; i < count; i++) {
        out[i] = (int16_t)(32000.f * triangle_wave(2 * M_PI * sineIdx)) *
                 this->channel3.volume;
        sineIdx += incr;
        if (sineIdx > 1.f)
            sineIdx -= 1.f;
    }
}

void apu_init(apu_t *apu) {
    this = apu;
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(4096);
    this->channel1.stream = LoadAudioStream(44100, 16, 1);
    this->channel2.stream = LoadAudioStream(44100, 16, 1);
    this->channel3.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(this->channel3.stream, triangle_callback);
    this->channel4.stream = LoadAudioStream(44100, 16, 1);
    this->channel5.stream = LoadAudioStream(44100, 16, 1);
}

void apu_status(uint8_t value) {
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
    this->channel1.volume_level_speed = value & 0xf;
}

void pulse_1_timer_low(uint8_t value) {
    this->channel1.timer &= 0xff00;
    this->channel1.timer |= value;
    this->channel1.frequency = 1790000.f / (16.f * (this->channel1.timer + 1));
}

void pulse_1_timer_high(uint8_t value) {
    this->channel1.timer &= 0xff;
    this->channel1.timer |= (value & 0x7) << 8;
    this->channel1.frequency = 1790000.f / (16.f * (this->channel1.timer + 1));
    if (this->channel1.enable) {
        this->channel1.length_counter = length_timer_lookup[value >> 3];
    }
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
    this->channel2.volume_level_speed = value & 0xf;
}

void pulse_2_timer_low(uint8_t value) {
    this->channel2.timer &= 0xff00;
    this->channel2.timer |= value;
    this->channel2.frequency = 1790000.f / (16.f * (this->channel2.timer + 1));
}

void pulse_2_timer_high(uint8_t value) {
    this->channel2.timer &= 0xff;
    this->channel2.timer |= (value & 0x7) << 8;
    this->channel2.frequency = 1790000.f / (16.f * (this->channel2.timer + 1));
    if (this->channel2.enable) {
        this->channel2.length_counter = length_timer_lookup[value >> 3];
    }
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
    this->channel3.frequency = 1790000.f / (32.f * (this->channel3.timer + 1));
}

void triangle_timer_high(uint8_t value) {
    this->channel3.timer &= 0xff;
    this->channel3.timer |= (value & 0x7) << 8;
    this->channel3.frequency = 1790000.f / (32.f * (this->channel3.timer + 1));
    if (this->channel3.enable) {
        this->channel3.length_counter = length_timer_lookup[value >> 3];
    }
}

void noise_config(uint8_t value) {
    this->channel4.loop = (value & 0x20) != 0;
    this->channel4.constant_volume = (value & 0x10) != 0;
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

void dpcm_direct_load(uint8_t value) {
    this->channel5.buffer = value & 0x7f;
    this->channel5.remaining_bits = 8;
}
