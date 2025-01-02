#ifndef M000_H
#define M000_H

#include <stdio.h>
#include "../types.h"

void m000_init(FILE*, uint8_t, uint8_t, nametable_arrangement);
uint8_t m000_cpu_read(uint16_t);
void m000_cpu_write(uint16_t, uint8_t);
uint8_t m000_ppu_read(uint16_t);
void m000_ppu_write(uint16_t, uint8_t);
void m000_free(void);

#endif
