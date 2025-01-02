#ifndef CPU_H
#define CPU_H

#include "types.h"

#define TO_U16(lsb, msb) ((lsb) | ((msb) << 8))
#define MSB(u16) (u16 >> 8)
#define LSB(u16) (u16 & 0xff)

void cpu_init(cpu_t *cpu);
cpu_t *get_cpu_handle(void);
void interrupt(interrupt_src src);
void push(uint8_t to_push);
void push_16le(uint16_t to_push);
uint8_t pop(void);
uint16_t pop_16le(void);

void lda(addressing_mode mode);
void ldx(addressing_mode mode);
void ldy(addressing_mode mode);
void cmp(addressing_mode mode, reg reg);
void branch(bool cond);
void transfer(reg from, reg to);
void jsr(void);
void rts(void);
void inc(addressing_mode mode);
void inc_r(reg reg);
void dec(addressing_mode mode);
void dec_r(reg reg);
void bit(addressing_mode mode);
void ora(addressing_mode mode);
void eor(addressing_mode mode);
void and (addressing_mode mode);
void adc(addressing_mode mode);
void sbc(addressing_mode mode);
void lsr(addressing_mode mode);
void lsr_a(void);
void asl(addressing_mode mode);
void asl_a(void);
void rol(addressing_mode mode);
void rol_a(void);
void ror(addressing_mode mode);
void ror_a(void);

#endif
