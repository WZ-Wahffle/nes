#ifndef M004_H
#define M004_H

#include <stdio.h>
#include "../types.h"

void m004_init(FILE*, uint8_t, uint8_t, nametable_arrangement, bool);
uint8_t m004_cpu_read(uint16_t);
void m004_cpu_write(uint16_t, uint8_t);
uint8_t m004_ppu_read(uint16_t);
void m004_ppu_write(uint16_t, uint8_t);
void m004_free(void);
void m004_scanline_callback(uint8_t);

#endif
