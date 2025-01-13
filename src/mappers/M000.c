#include "M000.h"
#include <stdlib.h>

static uint8_t *prg_ram;
static uint8_t prg_ram_size;
static uint8_t *prg_rom;
static uint8_t prg_rom_size;
static uint8_t *pattern_table;
static uint8_t *vram;
static uint8_t *name_table_1;
static uint8_t *name_table_2;
static nametable_arrangement arrangement;

void m000_init(FILE *f, uint8_t rom_size, uint8_t ram_size,
               nametable_arrangement arr) {
    prg_rom_size = rom_size;
    prg_ram_size = ram_size;
    if (prg_ram_size == 0) {
        prg_ram_size = 1;
    }
    if (prg_rom_size > 2) {
        printf("Invalid PRG ROM size\n");
        exit(1);
    }
    arrangement = arr;
    prg_ram = calloc(prg_ram_size * 8192, 1);
    prg_rom = calloc(16384 * prg_rom_size, 1);
    fread(prg_rom, 1, prg_rom_size * 16384, f);
    pattern_table = calloc(0x2000, 1);
    vram = calloc(0x800, 1);
    name_table_1 = vram;
    name_table_2 = vram + 0x400;
    fread(pattern_table, 1, 0x2000, f);
}

uint8_t m000_cpu_read(uint16_t addr) {
    if (addr >= 0x6000 && addr < 0x8000) {
        return prg_ram[addr - 0x6000];
    } else {
        return prg_rom[(addr - 0x8000) % (16384 * prg_rom_size)];
    }
}

void m000_cpu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x6000 && addr < 0x8000) {
        prg_ram[addr - 0x6000] = value;
    } else {
        printf("Attempted to write 0x%02x at ROM address 0x%04x\n", value,
               addr);
        exit(1);
    }
}

uint8_t m000_ppu_read(uint16_t addr) {
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

void m000_ppu_write(uint16_t addr, uint8_t value) {
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

void m000_free(void) {
    free(prg_ram);
    free(prg_rom);
    free(pattern_table);
    free(vram);
}

int32_t m000_get_bank_from_cpu_addr(uint16_t addr) {
    if(addr < 0x8000) return -1;
    return 0;
}
