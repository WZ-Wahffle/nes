#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <raylib.h>

typedef enum { EIGHTH, QUARTER, HALF, QUARTERINV } duty_cycle;

typedef struct {
    bool enable;
    duty_cycle duty;
    bool loop;
    bool constant_volume;
    uint8_t volume_level_speed;
    bool sweep_enable;
    uint8_t sweep_speed;
    bool sweep_negate;
    uint8_t sweep_shift_count;
    AudioStream stream;
    uint16_t timer;
    float frequency;
    uint16_t length_counter;
} pulse_channel;

typedef struct {
    bool enable;
    AudioStream stream;
    uint16_t length_counter;
    bool length_counter_halt;
    uint16_t linear_counter;
    uint16_t timer;
    float frequency;
    bool volume;
} triangle_channel;

typedef struct {
    bool enable;
    AudioStream stream;
    bool loop;
    bool constant_volume;
    uint8_t volume_level_speed;
    bool mode;
    uint16_t timer_period;
    uint16_t length_counter;
} noise_channel;

typedef struct {
    bool enable;
    uint8_t buffer;
    uint8_t remaining_bits;
    AudioStream stream;
} dpcm_channel;

typedef struct {
    pulse_channel channel1;
    pulse_channel channel2;
    triangle_channel channel3;
    noise_channel channel4;
    dpcm_channel channel5;
    bool long_sequence;
    bool interrupt_inhibit;
} apu_t;

typedef enum {
    VERTICAL, HORIZONTAL
} nametable_arrangement;

typedef struct {
    uint8_t (*read)(uint16_t);
    void (*write)(uint16_t, uint8_t);
} cartridge;

typedef struct {
    uint8_t ram[0x800];
    cartridge cart;
} cpu_mmu;

typedef struct {
    uint8_t palette_ram[0x20];
    cartridge cart;
} ppu_mmu;

typedef struct {
    uint8_t y_pos;
    uint8_t tile_index;
    uint8_t attributes;
    uint8_t x_pos;
} oam_entry;

typedef struct {
    ppu_mmu memory;
    bool enable_vblank_nmi;
    bool large_sprites;
    bool background_pattern_table_address;
    bool sprite_pattern_table_address;
    bool vram_address_increment;
    uint8_t base_nametable_address;
    volatile bool vblank;
    volatile bool sprite_0;
    bool sprite_overflow;
    bool w;
    uint16_t t;
    uint8_t scroll_x;
    uint8_t scroll_y;

    bool greyscale;
    bool show_background_leftmost;
    bool show_sprites_leftmost;
    bool background_enable;
    bool sprite_enable;
    bool red_emphasis;
    bool green_emphasis;
    bool blue_emphasis;

    uint8_t oam_addr;
    oam_entry oam[64];

    volatile bool joystick_strobe;
    volatile uint8_t joy1;
    volatile uint8_t joy2;
    volatile uint8_t joy1_mirror;
    volatile uint8_t joy2_mirror;
} ppu_t;

typedef struct {
    cpu_mmu memory;
    ppu_t *ppu;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint16_t pc;
    uint8_t sp;
    uint8_t p;

    volatile uint32_t remaining_cycles;

    volatile bool run;
    volatile bool step;
    int32_t run_until;
} cpu_t;

typedef enum { NMI, RESET, BRK } interrupt_src;

typedef enum {
    IMMEDIATE,
    ABSOLUTE,
    ZEROPAGE,
    ACCUMULATOR,
    IMPLIED,
    INDIRECTX,
    INDIRECTY,
    ZEROPAGEX,
    ABSX,
    ABSY,
    RELATIVE,
    INDIRECT,
    ZEROPAGEY
} addressing_mode;

typedef enum { C, Z, I, D, B, UNUSED, V, N } status_bit;

typedef enum { A, X, Y } reg;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t read_8(uint16_t addr);
uint16_t read_16le(uint16_t addr);
void write_8(uint16_t addr, uint8_t value);
void write_16le(uint16_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
