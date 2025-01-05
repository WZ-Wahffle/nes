#include "M004.h"
#include <stdlib.h>

static uint8_t prg_rom_size;
static uint8_t chr_rom_size;
static uint8_t *prg_rom;
static uint8_t *prg_ram;
static uint8_t *chr_rom;
static uint8_t r[8] = {0};
static uint8_t r_next = 0;
static bool prg_rom_bank_mode = 0;
static bool chr_inversion = 0;
static nametable_arrangement arrangement;
static uint8_t *vram;
static bool alt_nametable;
static bool irq = false;

void m004_init(FILE *f, uint8_t prg_size, uint8_t chr_size,
               nametable_arrangement arr, bool alt_nametable_layout) {
    prg_rom_size = prg_size;
    chr_rom_size = chr_size;
    prg_rom = calloc(16384 * prg_rom_size, 1);
    fread(prg_rom, 16384 * prg_rom_size, 1, f);
    chr_rom = calloc(8192 * chr_rom_size, 1);
    fread(chr_rom, 8192 * chr_rom_size, 1, f);
    prg_ram = calloc(8192, 1);
    arrangement = arr;
    if (alt_nametable_layout) {
        vram = calloc(0x1000, 1);
    } else {
        vram = calloc(0x800, 1);
    }
    alt_nametable = alt_nametable_layout;
}

uint8_t m004_cpu_read(uint16_t addr) {
    if (addr >= 0x6000 && addr < 0x8000) {
        return prg_ram[addr - 0x6000];
    }

    if (addr >= 0x8000) {
        if (prg_rom_bank_mode) {
            if (addr < 0xa000) {
                return prg_rom[((prg_rom_size * 2) - 2) * 0x2000 + addr -
                               0x8000];
            } else if (addr < 0xc000) {
                return prg_rom[r[7] * 0x2000 + addr - 0xa000];
            } else if (addr < 0xe000) {
                return prg_rom[r[6] * 0x2000 + addr - 0xc000];
            } else {
                return prg_rom[((prg_rom_size * 2) - 1) * 0x2000 + addr -
                               0xe000];
            }
        } else {
            if (addr < 0xa000) {
                return prg_rom[r[6] * 0x2000 + addr - 0x8000];
            } else if (addr < 0xc000) {
                return prg_rom[r[7] * 0x2000 + addr - 0xa000];
            } else if (addr < 0xe000) {
                return prg_rom[((prg_rom_size * 2) - 2) * 0x2000 + addr -
                               0xc000];
            } else {
                return prg_rom[((prg_rom_size * 2) - 1) * 0x2000 + addr -
                               0xe000];
            }
        }
    }

    printf("Attempted to read from cartridge CPU memory at 0x%04x\n", addr);
    exit(1);
}

void m004_cpu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x6000 && addr < 0x8000) {
        prg_ram[addr - 0x6000] = value;
    }

    if (addr >= 0x8000 && addr < 0xa000) {
        if (addr % 2 == 0) {
            r_next = value & 0b111;
            prg_rom_bank_mode = value & 0x40;
            chr_inversion = value & 0x80;
        } else {
            r[r_next] = value;
        }
    } else if (addr < 0xc000) {
        if (addr % 2 == 0) {
            arrangement = value & 1;
        } else {
            // write protection and RAM enable, omitted for now
        }
    } else if(addr < 0xe000) {
        if(addr % 2 == 0) {
            printf("TODO: IRQ latch\n");
        } else {
            printf("TODO: IRQ reload\n");
        }
    } else {
        irq = addr % 2;
    }

    if (addr < 0x6000) {
        printf("Attempted to write 0x%02x to cartridge CPU memory at 0x%04x\n",
               value, addr);
        exit(1);
    }
}

uint8_t m004_ppu_read(uint16_t addr) {
    if (addr < 0x2000) {
        if (chr_inversion) {
            if (addr < 0x400) {
                return chr_rom[r[2] * 1024 + addr];
            } else if (addr < 0x800) {
                return chr_rom[r[3] * 1024 + addr - 0x400];
            } else if (addr < 0xc00) {
                return chr_rom[r[4] * 1024 + addr - 0x800];
            } else if (addr < 0x1000) {
                return chr_rom[r[5] * 1024 + addr - 0xc00];
            } else if (addr < 0x1800) {
                return chr_rom[r[0] * 2048 + addr - 0x1000];
            } else {
                return chr_rom[r[1] * 2048 + addr - 0x1800];
            }
        } else {
            if (addr < 0x800) {
                return chr_rom[r[0] * 2048 + addr];
            } else if (addr < 0x1000) {
                return chr_rom[r[1] * 2048 + addr - 0x800];
            } else if (addr < 0x1400) {
                return chr_rom[r[2] * 1024 + addr - 0x1000];
            } else if (addr < 0x1800) {
                return chr_rom[r[3] * 1024 + addr - 0x1400];
            } else if (addr < 0x1c00) {
                return chr_rom[r[4] * 1024 + addr - 0x1800];
            } else {
                return chr_rom[r[5] * 1024 + addr - 0x1c00];
            }
        }
    } else if (addr < 0x3000) {
        if (alt_nametable) {
            return vram[addr - 0x2000];
        } else {
            if (arrangement == HORIZONTAL) {
                if (addr < 0x2400) {
                    return vram[addr - 0x2000];
                } else if (addr < 0x2800) {
                    return vram[addr - 0x2400];
                } else if (addr < 0x2c00) {
                    return vram[addr - 0x2800 + 0x400];
                } else {
                    return vram[addr - 0x2c00 + 0x400];
                }
            } else {
                if (addr < 0x2400) {
                    return vram[addr - 0x2000];
                } else if (addr < 0x2800) {
                    return vram[addr - 0x2400 + 0x400];
                } else if (addr < 0x2c00) {
                    return vram[addr - 0x2800];
                } else {
                    return vram[addr - 0x2c00 + 0x400];
                }
            }
        }
    } else {
        printf("Attempted to read from cartridge PPU memory at 0x%04x\n", addr);
        exit(1);
    }
}

void m004_ppu_write(uint16_t addr, uint8_t value) {
    if (addr >= 0x2000 && addr < 0x3000) {
        if (alt_nametable) {
            vram[addr - 0x2000] = value;
        } else {
            if (arrangement == HORIZONTAL) {
                if (addr < 0x2400) {
                    vram[addr - 0x2000] = value;
                } else if (addr < 0x2800) {
                    vram[addr - 0x2400] = value;
                } else if (addr < 0x2c00) {
                    vram[addr - 0x2800 + 0x400] = value;
                } else {
                    vram[addr - 0x2c00 + 0x400] = value;
                }
            } else {
                if (addr < 0x2400) {
                    vram[addr - 0x2000] = value;
                } else if (addr < 0x2800) {
                    vram[addr - 0x2400 + 0x400] = value;
                } else if (addr < 0x2c00) {
                    vram[addr - 0x2800] = value;
                } else {
                    vram[addr - 0x2c00 + 0x400] = value;
                }
            }
        }
    } else {
        printf("Attempted to write 0x%02x to cartridge PPU memory at 0x%04x\n",
               value, addr);
        exit(1);
    }
}

void m004_free(void) {
    free(prg_rom);
    free(chr_rom);
    free(prg_ram);
    free(vram);
}
