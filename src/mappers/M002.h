#ifndef M002_H
#define M002_H

#include <stdio.h>
#include "../types.h"

void m002_init(FILE*, uint8_t, nametable_arrangement);
uint8_t m002_cpu_read(uint16_t);
void m002_cpu_write(uint16_t, uint8_t);
uint8_t m002_ppu_read(uint16_t);
void m002_ppu_write(uint16_t, uint8_t);
void m002_free(void);

#endif
