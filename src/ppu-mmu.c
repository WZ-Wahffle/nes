#include "ppu-mmu.h"

void ppu_mmu_write(ppu_mmu *this, uint16_t addr, uint8_t value) {
    if(addr < 0x3f00) {
        this->cart.write(addr, value);
    } else {
        this->palette_ram[(addr - 0x3f00) % 0x20] = value;
    }
}
uint8_t ppu_mmu_read(ppu_mmu *this, uint16_t addr) {
    if(addr < 0x3f00) {
        return this->cart.read(addr);
    } else {
        return this->palette_ram[(addr - 0x3f00) % 0x20];
    }
}
