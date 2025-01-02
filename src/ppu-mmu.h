#ifndef PPU_MMU_H
#define PPU_MMU_H

#include "types.h"

void ppu_mmu_write(ppu_mmu *self, uint16_t addr, uint8_t value);
uint8_t ppu_mmu_read(ppu_mmu *self, uint16_t addr);

#endif
