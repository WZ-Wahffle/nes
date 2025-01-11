#include "M001.h"
#include <stdlib.h>

uint8_t shift = 0;
uint8_t shift_counter = 0;

uint8_t *prg_rom;

void m001_init(FILE *f, uint8_t prg_rom_size, uint8_t chr_rom_size, nametable_arrangement) {
    prg_rom = calloc(16384 * prg_rom_size, 1);
}
uint8_t m001_cpu_read(uint16_t) {}
void m001_cpu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x8000) {
        // Load
        if (value & 0x80) {
            shift = 0b10000;
            shift_counter = 0;
        } else {
            shift_counter++;
            shift = (shift >> 1) | ((value & 1) << 4) & 0b11111;
        }
    }

    if(addr >= 0x8000 && addr < 0xa000) {

    }
}
uint8_t m001_ppu_read(uint16_t) {}
void m001_ppu_write(uint16_t, uint8_t) {}
void m001_free(void) {}
