#include "M002.h"
#include <stdio.h>
#include <stdlib.h>

static uint8_t *prg_rom;
static uint8_t prg_rom_size;
static uint8_t *pattern_table;
static uint8_t *vram;
static uint8_t *name_table_1;
static uint8_t *name_table_2;
static nametable_arrangement arrangement;
static uint8_t bank_select = 0;

void m002_init(FILE *f, uint8_t prg_size, nametable_arrangement arr) {
    arrangement = arr;
    prg_rom_size = prg_size;
    prg_rom = calloc(0x4000 * prg_rom_size, 1);
    fread(prg_rom, 1, 0x4000 * prg_rom_size, f);
    pattern_table = calloc(0x2000, 1);
    vram = calloc(0x800, 1);
    name_table_1 = vram;
    name_table_2 = vram + 0x400;
    fread(pattern_table, 1, 0x2000, f);
}

uint8_t m002_cpu_read(uint16_t addr) {
    if(addr < 0x8000) {
        printf("Attempted to read from cartridge CPU memory at 0x%04x\n", addr);
        // exit(1);
        return 0;
    } else {
        if(addr < 0xc000) {
            return prg_rom[bank_select * 0x4000 + (addr % 0x4000)];
        } else {
            return prg_rom[(prg_rom_size-1) * 0x4000 + (addr % 0x4000)];
        }
    }
}

void m002_cpu_write(uint16_t addr, uint8_t value) {
    if(addr < 0x8000) {
        printf("Attempted to write 0x%02x to cartridge CPU memory at 0x%04x\n",
               value, addr);
        // exit(1);
    } else {
        bank_select = value;
    }
}

uint8_t m002_ppu_read(uint16_t addr) {
    if (addr < 0x2000) {
        return pattern_table[addr];
    } else if (addr >= 0x2000 && addr < 0x3000) {
        if (arrangement == HORIZONTAL) { // vertical mirroring
            if (addr < 0x2400) {
                return name_table_1[addr - 0x2000];
            } else if (addr < 0x2800) {
                return name_table_2[addr - 0x2400];
            } else if (addr < 0x2c00) {
                return name_table_1[addr - 0x2800];
            } else {
                return name_table_2[addr - 0x2c00];
            }
        } else { // horizontal mirroring
            if (addr < 0x2400) {
                return name_table_1[addr - 0x2000];
            } else if (addr < 0x2800) {
                return name_table_1[addr - 0x2400];
            } else if (addr < 0x2c00) {
                return name_table_2[addr - 0x2800];
            } else {
                return name_table_2[addr - 0x2c00];
            }
        }
    } else {
        printf("PPU reading at 0x%04x not allowed\n", addr);
        exit(1);
    }
}

void m002_ppu_write(uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        pattern_table[addr] = value;
    } else if (addr >= 0x2000 && addr < 0x3000) {
        if (arrangement == HORIZONTAL) { // vertical mirroring
            if (addr < 0x2400) {
                name_table_1[addr - 0x2000] = value;
            } else if (addr < 0x2800) {
                name_table_2[addr - 0x2400] = value;
            } else if (addr < 0x2c00) {
                name_table_1[addr - 0x2800] = value;
            } else {
                name_table_2[addr - 0x2c00] = value;
            }
        } else { // horizontal mirroring
            if (addr < 0x2400) {
                name_table_1[addr - 0x2000] = value;
            } else if (addr < 0x2800) {
                name_table_1[addr - 0x2400] = value;
            } else if (addr < 0x2c00) {
                name_table_2[addr - 0x2800] = value;
            } else {
                name_table_2[addr - 0x2c00] = value;
            }
        }
    } else {
        printf("PPU writing of value 0x%02x at 0x%04x not allowed\n", value,
               addr);
        exit(1);
    }
}
void m002_free(void) {
    free(prg_rom);
    free(pattern_table);
    free(vram);
}
