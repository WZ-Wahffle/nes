#include "cpu-mmu.h"
#include "apu.h"
#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>

void cpu_mmu_write(cpu_mmu *this, uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        this->ram[addr % 0x800] = value;
    }

    else if (addr >= 0x2000 && addr < 0x4000) {
        switch ((addr - 0x2000) % 8) {
        case 0x0:
            ppu_ctrl(value);
            break;
        case 0x1:
            ppu_mask(value);
            break;
        case 0x3:
            oam_addr(value);
            break;
        case 0x5:
            ppu_scroll(value);
            break;
        case 0x6:
            ppu_addr(value);
            break;
        case 0x7:
            ppu_data_write(value);
            break;
        default:
            printf("PPU write of 0x%02x at 0x%04x not implemented\n", value,
                   addr);
            exit(1);
        }
    }

    else if (addr >= 0x4000 && addr < 0x4018) {
        switch (addr - 0x4000) {
        case 0x00:
            pulse_1_config(value);
            break;
        case 0x01:
            pulse_1_sweep(value);
            break;
        case 0x02:
            pulse_1_timer_low(value);
            break;
        case 0x03:
            pulse_1_timer_high(value);
            break;
        case 0x04:
            pulse_2_config(value);
            break;
        case 0x05:
            pulse_2_sweep(value);
            break;
        case 0x06:
            pulse_2_timer_low(value);
            break;
        case 0x07:
            pulse_2_timer_high(value);
            break;
        case 0x08:
            triangle_linear_counter(value);
            break;
        case 0x09:
            break;
        case 0x0a:
            triangle_timer_low(value);
            break;
        case 0x0b:
            triangle_timer_high(value);
            break;
        case 0x0c:
            noise_config(value);
            break;
        case 0x0d:
            break;
        case 0x0e:
            noise_period(value);
            break;
        case 0x0f:
            noise_length_counter(value);
            break;
        case 0x10:
            dpcm_freq(value);
            break;
        case 0x11:
            dpcm_direct_load(value);
            break;
        case 0x12:
            dpcm_sample_addr(value);
            break;
        case 0x13:
            dpcm_sample_length(value);
            break;
        case 0x14:
            oam_dma(this, value);
            break;
        case 0x15:
            apu_status_write(value);
            break;
        case 0x16:
            joystick_strobe(value);
            break;
        case 0x17:
            frame_counter(value);
            break;
        default:
            printf("APU or IO write of 0x%02x at 0x%04x not implemented\n",
                   value, addr);
            exit(1);
        }
    }

    else if (addr >= 0x4020) {
        this->cart.write(addr, value);
    }

    else {
        printf("Invalid write of 0x%02x at 0x%04x\n", value, addr);
        exit(1);
    }
}
uint8_t cpu_mmu_read(cpu_mmu *this, uint16_t addr) {
    if (addr < 0x2000) {
        return this->ram[addr % 0x800];
    }

    else if (addr >= 0x2000 && addr < 0x4000) {
        switch ((addr - 0x2000) % 8) {
        case 2:
            return ppu_status();
            break;
        case 7:
            return ppu_data_read();
            break;
        default:
            printf("PPU read at 0x%04x not implemented\n", addr);
            return 0;
            exit(1);
        }
    }

    else if (addr >= 0x4000 && addr < 0x4018) {
        switch (addr - 0x4000) {
            case 0x15:
            return apu_status_read();
            break;
        case 0x16:
            return joystick_read_1();
            break;
        case 0x17:
            return joystick_read_2();
            break;
        default:
            printf("APU or IO read at 0x%04x not implemented\n", addr);
            exit(1);
        }
    }

    else if (addr >= 0x4020) {
        return this->cart.read(addr);
    }

    else {
        printf("Invalid read at 0x%04x\n", addr);
        exit(1);
    }
}
