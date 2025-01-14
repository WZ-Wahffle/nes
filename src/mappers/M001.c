#include "M001.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t shift = 0;
uint8_t shift_counter = 0;

uint8_t *prg_rom;
uint8_t prg_rom_size;
uint8_t *prg_ram;
uint8_t *chr_rom;
uint8_t chr_rom_size;
uint8_t *vram;
bool use_chr_ram;

enum m001_mirroring { ONE_LOWER, ONE_UPPER, HORI, VERT } mirroring = 0;
enum m001_prg_rom_bank_mode { LARGE_BANK, SWITCH_H, SWITCH_L } prg_rom_mode = 0;
bool small_chr_rom_banks = 0;

uint8_t chr_bank_0 = 0;
uint8_t chr_bank_1 = 0;
uint8_t prg_bank = 0;

void m001_init(FILE *f, uint8_t prg_size, uint8_t chr_size) {
    use_chr_ram = chr_size == 0;
    prg_rom_size = prg_size;
    prg_rom = calloc(0x4000 * prg_rom_size, 1);
    fread(prg_rom, 1, 0x4000 * prg_rom_size, f);
    prg_ram = calloc(0x2000, 1);
    if (use_chr_ram) {
        chr_rom_size = 4;
    } else {
        chr_rom_size = chr_size;
    }
    chr_rom = calloc(0x2000 * chr_rom_size, 1);
    fread(chr_rom, 1, 0x2000 * chr_rom_size, f);
    vram = calloc(0x800, 1);
}

uint8_t m001_cpu_read(uint16_t addr) {
    if (addr >= 0x6000 && addr < 0x8000) {
        return prg_ram[addr - 0x6000];
    }
    if (addr >= 0x8000) {
        switch (prg_rom_mode) {
        case LARGE_BANK:
            return prg_rom[(prg_bank & ~1) * 0x4000 + (addr % 0x8000)];
        case SWITCH_H:
            if (addr < 0xc000) {
                return prg_rom[addr % 0x4000];
            } else {
                return prg_rom[prg_bank * 0x4000 + (addr % 0x4000)];
            }
        case SWITCH_L:
            if (addr < 0xc000) {
                return prg_rom[prg_bank * 0x4000 + (addr % 0x4000)];
            } else {
                return prg_rom[(prg_rom_size - 1) * 0x4000 + (addr % 0x4000)];
            }
        }
    } else {
        printf("Attempted to read from cartridge CPU memory at 0x%04x\n", addr);
        return 0;
    }
}

void m001_cpu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x6000 && addr < 0x8000) {
        prg_ram[addr - 0x6000] = value;
    } else if (addr >= 0x8000) {
        // Load
        if (value & 0x80) {
            shift = 0b10000;
            shift_counter = 0;
        } else {
            shift_counter++;
            shift = ((shift >> 1) | ((value & 1) << 4)) & 0b11111;
            if (shift_counter == 5) {
                shift_counter = 0;
                if (addr < 0xa000) {
                    // Control
                    mirroring = shift & 0b11;
                    prg_rom_mode = ((shift & 0b1000) == 0)
                                       ? LARGE_BANK
                                       : (((shift >> 2) & 0b11) - 1);
                    small_chr_rom_banks = (shift & 0b10000) != 0;
                } else if (addr < 0xc000) {
                    chr_bank_0 = shift & 0x1f;
                } else if (addr < 0xe000) {
                    chr_bank_1 = shift & 0x1f;
                } else {
                    prg_bank = shift & 0xf;
                }
                shift = 0b10000;
            }
        }
    } else {
        printf("Attempted to write 0x%02x to cartridge CPU memory at 0x%04x\n",
               value, addr);
    }
}

uint8_t m001_ppu_read(uint16_t addr) {
    if (addr < 0x2000) {
        if (small_chr_rom_banks) {
            if (addr < 0x1000) {
                return chr_rom[chr_bank_0 * 0x1000 + (addr % 0x1000)];
            } else {
                return chr_rom[chr_bank_1 * 0x1000 + (addr % 0x1000)];
            }
        } else {
            return chr_rom[(chr_bank_0 & ~0) * 0x2000 + (addr % 0x2000)];
        }
    } else if (addr < 0x3000) {
        switch (mirroring) {
        case ONE_LOWER:
            return vram[(addr - 0x2000) % 0x400];
        case ONE_UPPER:
            return vram[(addr - 0x2000) % 0x400 + 0x400];
        case HORI:
            if (addr < 0x2400) {
                return vram[(addr % 0x400)];
            } else if (addr < 0x2800) {
                return vram[(addr % 0x400) + 0x400];
            } else if (addr < 0x2c00) {
                return vram[(addr % 0x400)];
            } else {
                return vram[(addr % 0x400) + 0x400];
            }
        case VERT:
            if (addr < 0x2400) {
                return vram[(addr % 0x400)];
            } else if (addr < 0x2800) {
                return vram[(addr % 0x400)];
            } else if (addr < 0x2c00) {
                return vram[(addr % 0x400) + 0x400];
            } else {
                return vram[(addr % 0x400) + 0x400];
            }
        }
    } else {
        printf("Attempted to read from cartridge PPU memory at 0x%04x\n", addr);
        return 0;
    }
}

void m001_ppu_write(uint16_t addr, uint8_t value) {
    if (addr < 0x2000 && use_chr_ram) {
        if (small_chr_rom_banks) {
            if (addr < 0x1000) {
                chr_rom[chr_bank_0 * 0x1000 + (addr % 0x1000)] = value;
            } else {
                chr_rom[chr_bank_1 * 0x1000 + (addr % 0x1000)] = value;
            }
        } else {
            chr_rom[(chr_bank_0 & ~0) * 0x2000 + (addr % 0x2000)] = value;
        }
    }
    if (addr >= 0x2000 && addr < 0x3000) {
        switch (mirroring) {
        case ONE_LOWER:
            vram[(addr - 0x2000) % 0x400] = value;
        case ONE_UPPER:
            vram[(addr - 0x2000) % 0x400 + 0x400] = value;
        case HORI:
            if (addr < 0x2400) {
                vram[(addr % 0x400)] = value;
            } else if (addr < 0x2800) {
                vram[(addr % 0x400) + 0x400] = value;
            } else if (addr < 0x2c00) {
                vram[(addr % 0x400)] = value;
            } else {
                vram[(addr % 0x400) + 0x400] = value;
            }
        case VERT:
            if (addr < 0x2400) {
                vram[(addr % 0x400)] = value;
            } else if (addr < 0x2800) {
                vram[(addr % 0x400)] = value;
            } else if (addr < 0x2c00) {
                vram[(addr % 0x400) + 0x400] = value;
            } else {
                vram[(addr % 0x400) + 0x400] = value;
            }
        }
    } else {
        printf("Attempted to write 0x%02x to cartridge PPU memory at 0x%04x\n",
               value, addr);
    }
}

void m001_free(void) {
    free(prg_rom);
    free(prg_ram);
    free(chr_rom);
    free(vram);
}

int32_t m001_get_bank_from_cpu_addr(uint16_t addr) {
    if (addr < 0x8000) {
        return -1;
    }

    switch (prg_rom_mode) {
    case LARGE_BANK:
        if (addr < 0xc000) {
            return (prg_bank & ~1);
        } else {
            return (prg_bank & ~1) + 1;
        }
    case SWITCH_H:
        if (addr < 0xc000) {
            return prg_rom_size - 1;
        } else {
            return prg_bank;
        }
    case SWITCH_L:
        if (addr < 0xc000) {
            return prg_bank;
        } else {
            return prg_rom_size - 1;
        }
    }

    return -1;
}
