#ifndef M001_H
#define M001_H

#include <stdio.h>
#include "../types.h"

void m001_init(FILE*, uint8_t, uint8_t, nametable_arrangement);
uint8_t m001_cpu_read(uint16_t);
void m001_cpu_write(uint16_t, uint8_t);
uint8_t m001_ppu_read(uint16_t);
void m001_ppu_write(uint16_t, uint8_t);
void m001_free(void);

#endif
