#ifndef PPU_H
#define PPU_H
#include <raylib.h>
#include "types.h"

#include "imgui/bridge.h"

void ppu_init(ppu_t* ppu);
void ppu_ctrl(uint8_t value);
void ppu_mask(uint8_t value);
uint8_t ppu_status(void);
void ppu_addr(uint8_t value);
uint8_t ppu_data_read(void);
void ppu_data_write(uint8_t value);
void ppu_scroll(uint8_t value);
void oam_addr(uint8_t value);
void oam_dma(cpu_mmu* mem, uint8_t value);
void joystick_strobe(uint8_t value);
uint8_t joystick_read_1(void);
uint8_t joystick_read_2(void);

void* ui_thread(void*);
uint8_t* fetch_background_pattern(uint16_t x, uint16_t y, uint8_t nametable_idx);
void fetch_tile(uint16_t idx);
uint8_t fetch_palette_index(uint8_t x, uint8_t y, uint8_t nametable_idx);
uint8_t fetch_palette_index_2(uint8_t x, uint8_t y);

#endif
