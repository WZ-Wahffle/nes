#include "cpu.h"
#include "cpu-mmu.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

static cpu_t *this;

// I hated every second of making these
uint8_t cycle_lookup[] = {
    7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 0, 8, 3, 3, 5, 5,
    3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, 2, 6, 2, 6, 3, 3, 3, 3,
    2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 0, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

void cpu_init(cpu_t *cpu) { this = cpu; }

cpu_t *get_cpu_handle(void) { return this; }

static uint8_t set_bit(uint8_t value, uint8_t idx) {
    return value | (1 << idx);
}

static uint8_t clear_bit(uint8_t value, uint8_t idx) {
    return value & ~(1 << idx);
}

static void set_status_bit(status_bit b, bool value) {
    this->p = value ? set_bit(this->p, b) : clear_bit(this->p, b);
}

static bool get_status_bit(status_bit b) { return (this->p & (1 << b)) != 0; }

uint8_t read_8(uint16_t addr) { return cpu_mmu_read(&this->memory, addr); }

uint16_t read_16le(uint16_t addr) {
    return TO_U16(read_8(addr), read_8(addr + 1));
}

void write_8(uint16_t addr, uint8_t value) {
    cpu_mmu_write(&this->memory, addr, value);
}

void write_16le(uint16_t addr, uint8_t value) {
    write_8(TO_U16(read_8(addr), read_8(addr + 1)), value);
}

static uint8_t read_next_byte(void) { return read_8(this->pc++); }

static uint16_t read_next_short_le(void) {
    uint16_t ret = read_8(this->pc) | (read_8(this->pc + 1) << 8);
    this->pc += 2;
    return ret;
}

static uint16_t read_mode_addr(addressing_mode mode) {
    switch (mode) {
    case ABSOLUTE:
        return read_next_short_le();
        break;
    case ZEROPAGE:
        return read_next_byte();
        break;
    case ZEROPAGEX:
        return (this->x + read_next_byte()) % 0x100;
        break;
    case ABSX:
        return (this->x + read_next_short_le()) % 0x10000;
        break;
    default:
        printf("Invalid read mode at line %d\n", __LINE__);
        exit(1);
    }
}

static uint8_t read_mode(addressing_mode mode) {
    switch (mode) {
    case ACCUMULATOR:
        return this->a;
        break;
    case IMMEDIATE:
        return read_next_byte();
        break;
    case ABSOLUTE:
        return read_8(read_next_short_le());
        break;
    case ZEROPAGE:
        return read_8(read_next_byte());
        break;
    case INDIRECTX:
        return read_16le(read_8((this->x + read_next_byte()) % 0x100));
        break;
    case INDIRECTY:
        return read_8((this->y + read_16le(read_next_byte())) % 0x10000);
        break;
    case ZEROPAGEX:
        return read_8((this->x + read_next_byte()) % 0x100);
        break;
    case ABSX:
        return read_8((this->x + read_next_short_le()) % 0x10000);
        break;
    case ABSY:
        return read_8((this->y + read_next_short_le()) % 0x10000);
        break;
    case ZEROPAGEY:
        return read_8((this->y + read_next_byte()) % 0x100);
        break;
    default:
        printf("Invalid read mode\n");
        exit(1);
    }
}

static void write_mode(addressing_mode mode, uint8_t value) {
    switch (mode) {
    case ABSOLUTE:
        write_8(read_next_short_le(), value);
        break;
    case ZEROPAGE:
        write_8(read_next_byte(), value);
        break;
    case ACCUMULATOR:
        this->a = value;
        break;
    case INDIRECTX:
        write_16le(read_8((this->x + read_next_byte()) % 0x100),
                   value); // TODO test this
        break;
    case INDIRECTY:
        write_8((this->y + read_16le(read_next_byte())) % 0x10000,
                value); // TODO test this
        break;
    case ZEROPAGEX:
        write_8((this->x + read_next_byte()) % 0x100, value);
        break;
    case ABSX:
        write_8((this->x + read_next_short_le()) % 0x10000, value);
        break;
    case ABSY:
        write_8((this->y + read_next_short_le()) % 0x10000, value);
        break;
    case ZEROPAGEY:
        write_8((this->y + read_next_byte()) % 0x100, value);
        break;
    default:
        printf("Invalid write mode at line %d\n", __LINE__);
        exit(1);
    }
}

static void execute(uint8_t opcode) {
    switch (opcode) {
    case 0x01:
        ora(INDIRECTX);
        break;
    case 0x05:
        ora(ZEROPAGE);
        break;
    case 0x06:
        asl(ZEROPAGE);
        break;
    case 0x09:
        ora(IMMEDIATE);
        break;
    case 0x0a:
        asl_a();
        break;
    case 0x0d:
        ora(ABSOLUTE);
        break;
    case 0x0e:
        asl(ABSOLUTE);
        break;
    case 0x10:
        branch(!get_status_bit(N));
        break;
    case 0x11:
        ora(INDIRECTY);
        break;
    case 0x15:
        ora(ZEROPAGEX);
        break;
    case 0x16:
        asl(ZEROPAGEX);
        break;
    case 0x18:
        set_status_bit(C, false);
        break;
    case 0x19:
        ora(ABSY);
        break;
    case 0x1d:
        ora(ABSX);
        break;
    case 0x1e:
        asl(ABSX);
        break;
    case 0x20:
        jsr();
        break;
    case 0x21:
        and(INDIRECTX);
        break;
    case 0x24:
        bit(ZEROPAGE);
        break;
    case 0x25:
        and(ZEROPAGE);
        break;
    case 0x26:
        rol(ZEROPAGE);
        break;
    case 0x29:
        and(IMMEDIATE);
        break;
    case 0x2a:
        rol_a();
        break;
    case 0x2c:
        bit(ABSOLUTE);
        break;
    case 0x2d:
        and(ABSOLUTE);
        break;
    case 0x2e:
        rol(ABSOLUTE);
        break;
        case 0x30:
        branch(get_status_bit(N));
        break;
    case 0x31:
        and(INDIRECTY);
        break;
    case 0x35:
        and(ZEROPAGEX);
        break;
    case 0x36:
        rol(ZEROPAGE);
        break;
    case 0x38:
        set_status_bit(C, true);
        break;
    case 0x39:
        and(ABSY);
        break;
    case 0x3d:
        and(ABSX);
        break;
    case 0x3e:
        rol(ABSX);
        break;
    case 0x40:
        this->p = pop();
        this->pc = pop_16le() + 1;
        break;
    case 0x41:
        eor(INDIRECTX);
        break;
    case 0x45:
        eor(ZEROPAGE);
        break;
    case 0x46:
        lsr(ZEROPAGE);
        break;
    case 0x48:
        push(this->a);
        break;
    case 0x49:
        eor(IMMEDIATE);
        break;
    case 0x4a:
        lsr_a();
        break;
    case 0x4c:
        this->pc = read_next_short_le();
        break;
    case 0x4d:
        eor(ABSOLUTE);
        break;
    case 0x4e:
        lsr(ABSOLUTE);
        break;
    case 0x51:
        eor(INDIRECTY);
        break;
    case 0x55:
        eor(ZEROPAGEX);
        break;
    case 0x56:
        lsr(ZEROPAGEX);
        break;
    case 0x59:
        eor(ABSY);
        break;
    case 0x5d:
        eor(ABSX);
        break;
    case 0x5e:
        lsr(ABSX);
        break;
    case 0x60:
        rts();
        break;
    case 0x61:
        adc(INDIRECTX);
        break;
    case 0x65:
        adc(ZEROPAGE);
        break;
    case 0x66:
        ror(ZEROPAGE);
        break;
    case 0x68:
        this->a = pop();
        set_status_bit(Z, this->a == 0);
        set_status_bit(N, (this->a & 0x80) != 0);
        break;
    case 0x69:
        adc(IMMEDIATE);
        break;
    case 0x6a:
        ror_a();
        break;
    case 0x6c:
        this->pc = read_16le(read_next_short_le());
        break;
    case 0x6d:
        adc(ABSOLUTE);
        break;
    case 0x6e:
        ror(ABSOLUTE);
        break;
    case 0x71:
        adc(INDIRECTY);
        break;
    case 0x75:
        adc(ZEROPAGEX);
        break;
    case 0x76:
        ror(ZEROPAGEX);
        break;
    case 0x78:
        set_status_bit(I, true);
        break;
    case 0x79:
        adc(ABSY);
        break;
    case 0x7d:
        adc(ABSX);
        break;
    case 0x7e:
        ror(ABSX);
        break;
    case 0x81:
        write_mode(INDIRECTX, this->a);
        break;
    case 0x84:
        write_mode(ZEROPAGE, this->y);
        break;
    case 0x85:
        write_mode(ZEROPAGE, this->a);
        break;
    case 0x86:
        write_mode(ZEROPAGE, this->x);
        break;
    case 0x88:
        dec_r(Y);
        break;
    case 0x8a:
        transfer(X, A);
        break;
    case 0x8c:
        write_mode(ABSOLUTE, this->y);
        break;
    case 0x8d:
        write_mode(ABSOLUTE, this->a);
        break;
    case 0x8e:
        write_mode(ABSOLUTE, this->x);
        break;
    case 0x90:
        branch(!get_status_bit(C));
        break;
    case 0x91:
        write_mode(INDIRECTY, this->a);
        break;
    case 0x94:
        write_mode(ZEROPAGEX, this->y);
        break;
    case 0x95:
        write_mode(ZEROPAGEX, this->a);
        break;
    case 0x96:
        write_mode(ZEROPAGEY, this->x);
        break;
    case 0x98:
        transfer(Y, A);
        break;
    case 0x99:
        write_mode(ABSY, this->a);
        break;
    case 0x9a:
        this->sp = this->x;
        break;
    case 0x9d:
        write_mode(ABSX, this->a);
        break;
    case 0xa0:
        ldy(IMMEDIATE);
        break;
    case 0xa1:
        lda(INDIRECTX);
        break;
    case 0xa2:
        ldx(IMMEDIATE);
        break;
    case 0xa4:
        ldy(ZEROPAGE);
        break;
    case 0xa5:
        lda(ZEROPAGE);
        break;
    case 0xa6:
        ldx(ZEROPAGE);
        break;
    case 0xa8:
        transfer(A, Y);
        break;
    case 0xa9:
        lda(IMMEDIATE);
        break;
    case 0xaa:
        transfer(A, X);
        break;
    case 0xac:
        ldy(ABSOLUTE);
        break;
    case 0xad:
        lda(ABSOLUTE);
        break;
    case 0xae:
        ldx(ABSOLUTE);
        break;
    case 0xb0:
        branch(get_status_bit(C));
        break;
    case 0xb1:
        lda(INDIRECTY);
        break;
    case 0xb4:
        ldy(ZEROPAGEX);
        break;
    case 0xb5:
        lda(ZEROPAGEX);
        break;
    case 0xb6:
        ldx(ZEROPAGEY);
        break;
    case 0xb9:
        lda(ABSY);
        break;
    case 0xba:
        this->x = this->sp;
        set_status_bit(N, (this->x & 0x80) != 0);
        set_status_bit(Z, this->x == 0);
        break;
    case 0xbc:
        ldy(ABSX);
        break;
    case 0xbd:
        lda(ABSX);
        break;
    case 0xbe:
        ldx(ABSY);
        break;
    case 0xc0:
        cmp(IMMEDIATE, Y);
        break;
    case 0xc1:
        cmp(INDIRECTX, A);
        break;
    case 0xc4:
        cmp(ZEROPAGE, Y);
        break;
    case 0xc5:
        cmp(ZEROPAGE, A);
        break;
    case 0xc6:
        dec(ZEROPAGE);
        break;
    case 0xc8:
        inc_r(Y);
        break;
    case 0xc9:
        cmp(IMMEDIATE, A);
        break;
    case 0xca:
        dec_r(X);
        break;
    case 0xcc:
        cmp(ABSOLUTE, Y);
        break;
    case 0xcd:
        cmp(ABSOLUTE, A);
        break;
    case 0xce:
        dec(ABSOLUTE);
        break;
    case 0xd0:
        branch(!get_status_bit(Z));
        break;
    case 0xd1:
        cmp(INDIRECTY, A);
        break;
    case 0xd5:
        cmp(ZEROPAGEX, A);
        break;
    case 0xd6:
        dec(ZEROPAGEX);
        break;
    case 0xd8:
        set_status_bit(D, false);
        break;
    case 0xd9:
        cmp(ABSY, A);
        break;
    case 0xdd:
        cmp(ABSX, A);
        break;
    case 0xde:
        dec(ABSX);
        break;
    case 0xe0:
        cmp(IMMEDIATE, X);
        break;
    case 0xe1:
        sbc(INDIRECTX);
        break;
    case 0xe4:
        cmp(ZEROPAGE, X);
        break;
    case 0xe5:
        sbc(ZEROPAGE);
        break;
    case 0xe6:
        inc(ZEROPAGE);
        break;
    case 0xe8:
        inc_r(X);
        break;
    case 0xe9:
        sbc(IMMEDIATE);
        break;
    case 0xec:
        cmp(ABSOLUTE, X);
        break;
    case 0xed:
        sbc(ABSOLUTE);
        break;
    case 0xee:
        inc(ABSOLUTE);
        break;
    case 0xf0:
        branch(get_status_bit(Z));
        break;
    case 0xf1:
        sbc(INDIRECTY);
        break;
    case 0xf5:
        sbc(ZEROPAGEX);
        break;
    case 0xf6:
        inc(ZEROPAGEX);
        break;
    case 0xf9:
        sbc(ABSY);
        break;
    case 0xfd:
        sbc(ABSX);
        break;
    case 0xfe:
        inc(ABSX);
        break;
    default:
        printf("Opcode 0x%02x at 0x%04x not implemented\n", opcode,
               this->pc - 1);
        exit(1);
    }
}

static void run(void) {
    while (1) {
        if (this->step || this->run) {
            this->step = false;

            uint8_t opcode = read_next_byte();
            execute(opcode);
            if (this->ppu->enable_vblank_nmi && this->ppu->vblank) {
                this->ppu->vblank = false;
                push_16le(this->pc - 1);
                push(this->p);
                set_status_bit(I, true);
                this->pc = read_16le(0xfffa);
            }

            this->remaining_cycles -= cycle_lookup[opcode];

            if (this->pc == this->run_until) {
                this->run = false;
                this->step = false;
            }
        }
    }
}

void interrupt(interrupt_src src) {
    switch (src) {
    case NMI:
        break;
    case RESET:
        this->pc = read_8(0xfffc) | (read_8(0xfffd) << 8);
        run();
        break;
    case BRK:
        break;
    }
}

void lda(addressing_mode mode) {
    this->a = read_mode(mode);
    set_status_bit(N, (this->a & 0x80) != 0);
    set_status_bit(Z, this->a == 0);
}

void ldx(addressing_mode mode) {
    this->x = read_mode(mode);
    set_status_bit(N, (this->x & 0x80) != 0);
    set_status_bit(Z, this->x == 0);
}

void ldy(addressing_mode mode) {
    this->y = read_mode(mode);
    set_status_bit(N, (this->y & 0x80) != 0);
    set_status_bit(Z, this->y == 0);
}

void cmp(addressing_mode mode, reg reg) {
    uint8_t first;
    switch (reg) {
    case A:
        first = this->a;
        break;
    case X:
        first = this->x;
        break;
    case Y:
        first = this->y;
        break;
    }
    uint8_t second = read_mode(mode);
    uint8_t result = first - second;

    set_status_bit(Z, result == 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(C, first >= second);
}

void branch(bool cond) {
    int8_t offset = read_next_byte();
    if (cond) {
        this->pc += offset;
    }
}

void transfer(reg from, reg to) {
    uint8_t result;
    switch (from) {
    case A:
        result = this->a;
        break;
    case X:
        result = this->x;
        break;
    case Y:
        result = this->y;
        break;
    }

    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);

    switch (to) {
    case A:
        this->a = result;
        break;
    case X:
        this->x = result;
        break;
    case Y:
        this->y = result;
        break;
    }
}

void jsr(void) {
    uint16_t target = read_next_short_le();
    push_16le(this->pc - 1);
    this->pc = target;
}

void rts(void) { this->pc = pop_16le() + 1; }

void inc(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t result = read_8(addr) + 1;
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    write_8(addr, result);
}

void inc_r(reg reg) {
    uint8_t result;
    switch (reg) {
    case X:
        result = ++this->x;
        break;
    case Y:
        result = ++this->y;
        break;
    default:;
    }

    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
}

void dec(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t result = read_8(addr) - 1;
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    write_8(addr, result);
}

void dec_r(reg reg) {
    uint8_t result;
    switch (reg) {
    case X:
        result = --this->x;
        break;
    case Y:
        result = --this->y;
        break;
    default:;
    }

    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
}

void bit(addressing_mode mode) {
    uint8_t operand = read_mode(mode);
    set_status_bit(Z, (operand & this->a) == 0);
    set_status_bit(N, (operand & 0x40) != 0);
    set_status_bit(V, (operand & 0x20) != 0);
}

void ora(addressing_mode mode) {
    this->a |= read_mode(mode);
    set_status_bit(N, (this->a & 0x80) != 0);
    set_status_bit(Z, this->a == 0);
}

void eor(addressing_mode mode) {
    this->a ^= read_mode(mode);
    set_status_bit(N, (this->a & 0x80) != 0);
    set_status_bit(Z, this->a == 0);
}

void and (addressing_mode mode) {
    this->a &= read_mode(mode);
    set_status_bit(N, (this->a & 0x80) != 0);
    set_status_bit(Z, this->a == 0);
}

void adc(addressing_mode mode) {
    uint8_t operand = read_mode(mode);
    uint16_t result = this->a + operand + (get_status_bit(C) ? 1 : 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    set_status_bit(C, result > 0xff);
    set_status_bit(V,
                   !((this->a ^ operand) & 0x80) &&
                       ((this->a ^ result) & 0x80)); // TODO: test this
    this->a = result & 0xff;
}

void sbc(addressing_mode mode) {
    uint8_t operand = read_mode(mode);
    uint16_t result = this->a - operand - (get_status_bit(C) ? 0 : 1);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    set_status_bit(C, result < 0x100);
    set_status_bit(V,
                   ((this->a ^ operand) & 0x80) &&
                       ((this->a ^ result) & 0x80)); // TODO: test this
    this->a = result & 0xff;
}

void lsr(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t operand = read_8(addr);
    write_8(addr, operand / 2);
    set_status_bit(C, operand & 1);
    set_status_bit(N, false);
    set_status_bit(Z, operand / 2 == 0);
}

void lsr_a(void) {
    set_status_bit(C, this->a & 1);
    set_status_bit(N, false);
    this->a /= 2;
    set_status_bit(Z, this->a == 0);
}

void asl(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t operand = read_8(addr);
    write_8(addr, operand * 2);
    set_status_bit(C, (operand & 0x80) != 0);
    set_status_bit(N, (operand & 0x40) != 0);
    set_status_bit(Z, operand / 2 == 0);
}

void asl_a(void) {
    set_status_bit(C, (this->a & 0x80) != 0);
    set_status_bit(N, (this->a & 0x40) != 0);
    this->a *= 2;
    set_status_bit(Z, this->a == 0);
}

void rol(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t operand = read_8(addr);
    uint8_t result = (operand * 2) | (get_status_bit(C) ? 1 : 0);
    write_8(addr, result);
    set_status_bit(C, (operand & 0x80) != 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
}

void rol_a(void) {
    uint8_t result = (this->a * 2) | (get_status_bit(C) ? 1 : 0);
    set_status_bit(C, (this->a & 0x80) != 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    this->a = result;
}

void ror(addressing_mode mode) {
    uint16_t addr = read_mode_addr(mode);
    uint8_t operand = read_8(addr);
    uint8_t result = (operand / 2) | (get_status_bit(C) ? 0x80 : 0);
    write_8(addr, result);
    set_status_bit(C, (operand & 1) != 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
}

void ror_a(void) {
    uint8_t result = (this->a / 2) | (get_status_bit(C) ? 0x80 : 0);
    set_status_bit(C, (this->a & 1) != 0);
    set_status_bit(N, (result & 0x80) != 0);
    set_status_bit(Z, result == 0);
    this->a = result;
}

void push(uint8_t to_push) { write_8(0x100 + (this->sp--), to_push); }
void push_16le(uint16_t to_push) {
    push(MSB(to_push));
    push(LSB(to_push));
}
uint8_t pop(void) { return read_8(0x100 + (++this->sp)); }
uint16_t pop_16le(void) {
    uint8_t lsb = pop();
    uint8_t msb = pop();
    return TO_U16(lsb, msb);
}
