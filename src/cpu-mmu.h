#ifndef CPU_MMU_H
#define CPU_MMU_H

#include "types.h"

void cpu_mmu_write(cpu_mmu *self, uint16_t addr, uint8_t value);
uint8_t cpu_mmu_read(cpu_mmu *self, uint16_t addr);

#endif
