#include "ppu.h"
#include "cpu-mmu.h"
#include "cpu.h"
#include "imgui/bridge.h"
#include "ppu-mmu.h"
#include <linux/limits.h>
#include <raylib.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define VIEWPORT_WIDTH 256
#define VIEWPORT_HEIGHT 240
#define WINDOW_WIDTH VIEWPORT_WIDTH * 4
#define WINDOW_HEIGHT VIEWPORT_HEIGHT * 4
#define CYCLES_PER_LINE 113

uint32_t *framebuffer = NULL;
static ppu_t *this;

static uint32_t palette_lookup[] = {
    0xff626262, 0xffB21F00, 0xffC80424, 0xffB20052, 0xff760073, 0xff240080,
    0xff000B73, 0xff002852, 0xff004424, 0xff005700, 0xff005C00, 0xff245300,
    0xff763C00, 0xff000000, 0xff000000, 0xff000000, 0xffABABAB, 0xffFF570D,
    0xffFF304B, 0xffFF138A, 0xffD608BC, 0xff6912D2, 0xff002EC7, 0xff00549D,
    0xff007B60, 0xff009820, 0xff00A300, 0xff429900, 0xffB47D00, 0xff000000,
    0xff000000, 0xff000000, 0xffFFFFFF, 0xffFFAE53, 0xffFF8590, 0xffFF65D3,
    0xffFF57FF, 0xffCF5DFF, 0xff5777FF, 0xff009EFA, 0xff00C7BD, 0xff00E77A,
    0xff11F643, 0xff7EEF26, 0xffF6D52C, 0xff4E4E4E, 0xff000000, 0xff000000,
    0xffFFFFFF, 0xffFFE1B6, 0xffFFD1CE, 0xffFFC3E9, 0xffFFBCFF, 0xffF4BDFF,
    0xffC3C6FF, 0xff9AD5FF, 0xff81E6E9, 0xff81F4CE, 0xff9AFBB6, 0xffC3FAA9,
    0xffF4F0A9, 0xffB8B8B8, 0xff000000, 0xff000000};

void ppu_init(ppu_t *ppu) { this = ppu; }

void ppu_ctrl(uint8_t value) {
    this->base_nametable_address = value & 0b11;
    this->vram_address_increment = (value & 0b100) != 0;
    this->sprite_pattern_table_address = (value & 0b1000) != 0;
    this->background_pattern_table_address = (value & 0b10000) != 0;
    this->large_sprites = (value & 0b100000) != 0;
    this->enable_vblank_nmi = (value & 0b10000000) != 0;
}

void ppu_mask(uint8_t value) {
    this->greyscale = value & 0b1;
    this->show_background_leftmost = value & 0b10;
    this->show_sprites_leftmost = value & 0b100;
    this->background_enable = value & 0b1000;
    this->sprite_enable = value & 0b10000;
    this->red_emphasis = value & 0b100000;
    this->green_emphasis = value & 0b1000000;
    this->blue_emphasis = value & 0b10000000;
}

uint8_t ppu_status(void) {
    this->w = false;
    uint8_t ret = (this->vblank ? 0x80 : 0) | (this->sprite_0 ? 0x40 : 0) |
                  (this->sprite_overflow ? 0x20 : 0);
    this->vblank = false;
    return ret;
}

void ppu_addr(uint8_t value) {
    if (!this->w) {
        this->t &= 0xff;
        this->t |= value << 8;
        this->t &= 0x3fff;
        this->w = true;
    } else {
        this->t &= 0xff00;
        this->t |= value;
        this->w = false;
    }
}

uint8_t ppu_data_read(void) {
    static uint8_t data_read_buffer = 0;
    uint8_t ret = data_read_buffer;
    data_read_buffer = ppu_mmu_read(&this->memory, this->t);
    this->t += this->vram_address_increment ? 32 : 1;
    return ret;
}

void ppu_data_write(uint8_t value) {
    ppu_mmu_write(&this->memory, this->t, value);
    this->t += this->vram_address_increment ? 32 : 1;
}

void ppu_scroll(uint8_t value) {
    if (this->w) {
        this->scroll_y = value;
        this->w = false;
    } else {
        this->scroll_x = value;
        this->w = true;
    }
}

void oam_addr(uint8_t value) { this->oam_addr = value; }

void oam_dma(cpu_mmu *mem, uint8_t value) {
    for (uint8_t i = 0; i < 0x40; i++) {
        this->oam[i].y_pos = cpu_mmu_read(mem, (value << 8) + 4 * i);
        this->oam[i].tile_index = cpu_mmu_read(mem, (value << 8) + 4 * i + 1);
        this->oam[i].attributes = cpu_mmu_read(mem, (value << 8) + 4 * i + 2);
        this->oam[i].x_pos = cpu_mmu_read(mem, (value << 8) + 4 * i + 3);
    }
}

void joystick_strobe(uint8_t value) {
    this->joy1 = this->joy1_mirror;
    this->joystick_strobe = value != 0;
}

uint8_t joystick_read_1(void) {
    uint8_t ret = this->joy1 & 1;
    this->joy1 >>= 1;
    return ret;
}

uint8_t joystick_read_2(void) {
    uint8_t ret = this->joy2 & 1;
    this->joy2 >>= 1;
    return ret;
}

static uint8_t pattern_buffer[512][8][8] = {0};

void *ui_thread(void *_) {
    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "nes");

    framebuffer = malloc(VIEWPORT_WIDTH * VIEWPORT_HEIGHT * 4);
    Image framebufferImage = {framebuffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1,
                              PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture texture = LoadTextureFromImage(framebufferImage);

    bool sprite_0_hit_buffer[VIEWPORT_HEIGHT][VIEWPORT_WIDTH] = {0};

    bool show_debug = true;

    uint8_t joy1_idx = 0;

    cpp_init();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        for (uint32_t i = 0; i < 512; i++) {
            fetch_tile(i);
        }

        this->joy1_mirror =
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)
             << 0) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)
             << 1) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_MIDDLE_LEFT) << 2) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_MIDDLE_RIGHT) << 3) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_LEFT_FACE_UP) << 4) |
            ((GetGamepadAxisMovement(joy1_idx, GAMEPAD_AXIS_LEFT_Y) < -0.5)
             << 4) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_LEFT_FACE_DOWN)
             << 5) |
            ((GetGamepadAxisMovement(joy1_idx, GAMEPAD_AXIS_LEFT_Y) > 0.5)
             << 5) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_LEFT_FACE_LEFT)
             << 6) |
            ((GetGamepadAxisMovement(joy1_idx, GAMEPAD_AXIS_LEFT_X) < -0.5)
             << 6) |
            (IsGamepadButtonDown(joy1_idx, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)
             << 7) |
            ((GetGamepadAxisMovement(joy1_idx, GAMEPAD_AXIS_LEFT_X) > 0.5)
             << 7);

        this->joy2_mirror =
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) << 0) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) << 1) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_MIDDLE_LEFT) << 2) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_MIDDLE_RIGHT) << 3) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_LEFT_FACE_UP) << 4) |
            ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_LEFT_Y) < -0.5) << 4) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_LEFT_FACE_DOWN) << 5) |
            ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_LEFT_Y) > 0.5) << 5) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_LEFT_FACE_LEFT) << 6) |
            ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_LEFT_X) < -0.5) << 6) |
            (IsGamepadButtonDown(1, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) << 7) |
            ((GetGamepadAxisMovement(1, GAMEPAD_AXIS_LEFT_X) > 0.5) << 7);

        for (uint32_t i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; i++) {
            framebuffer[i] = palette_lookup[this->memory.palette_ram[16]];
        }

        this->sprite_0 = false;
        get_cpu_handle()->remaining_cycles += CYCLES_PER_LINE;
        this->vblank = false;

        for (uint16_t y = 0; y < VIEWPORT_HEIGHT; y++) {
            // background drawing
            uint16_t scrolled_y = (y + this->scroll_y +
                                   256 * (this->base_nametable_address / 2)) %
                                  480;
            for (uint16_t x = 0; x < VIEWPORT_WIDTH; x++) {
                uint16_t scrolled_x =
                    (x + this->scroll_x +
                     256 * (this->base_nametable_address % 2)) %
                    512;
                // pattern for current tile
                uint8_t *pattern =
                    fetch_background_pattern(scrolled_x, scrolled_y);

                uint8_t palette_idx =
                    fetch_palette_index(scrolled_x, scrolled_y);

                if (pattern[(scrolled_x % 8) + 8 * (scrolled_y % 8)] != 0) {
                    framebuffer[x + y * VIEWPORT_WIDTH] =
                        palette_lookup[this->memory.palette_ram
                                           [palette_idx * 4 +
                                            pattern[(scrolled_x % 8) +
                                                    8 * (scrolled_y % 8)]]];
                    sprite_0_hit_buffer[y][x] = true;
                }
            }

            // sprite drawing
            for (int8_t i = 63; i >= 0; i--) {
                if (this->oam[i].y_pos <= y && this->oam[i].y_pos + 8 > y) {
                    if (this->large_sprites) {
                        printf("aaa\n");
                        break;
                    }
                    bool flip_x = this->oam[i].attributes & 0x40;
                    bool flip_y = this->oam[i].attributes & 0x80;
                    uint8_t palette_idx = this->oam[i].attributes & 0x3;
                    bool in_front = this->oam[i].attributes & 0x20;
                    uint8_t tile_y = y - (this->oam[i].y_pos);
                    uint8_t *pattern = pattern_buffer
                        [(this->sprite_pattern_table_address ? 256 : 0) +
                         this->oam[i].tile_index]
                        [flip_y ? (7 - (tile_y % 8)) : (tile_y % 8)];

                    for (uint8_t x = 0; x < 8; x++) {
                        uint16_t tile_x = flip_x ? (7 - x) : (x);
                        if (i == 0 &&
                            sprite_0_hit_buffer[y][(this->oam[i].x_pos + x) %
                                                   256] &&
                            pattern[tile_x] != 0) {
                            this->sprite_0 = true;
                        }

                        if ((!sprite_0_hit_buffer[y][(this->oam[i].x_pos + x) %
                                                     256] ||
                             !in_front) &&
                            pattern[tile_x] != 0) {
                            framebuffer[VIEWPORT_WIDTH * y +
                                        (this->oam[i].x_pos + x) % 256] =
                                palette_lookup[this->memory.palette_ram
                                                   [16 + palette_idx * 4 +
                                                    pattern[tile_x]]];
                        }
                    }
                }
            }

            get_cpu_handle()->remaining_cycles += CYCLES_PER_LINE;
        }

        for (uint16_t x = 0; x < VIEWPORT_WIDTH; x++) {
            for (uint16_t y = 0; y < VIEWPORT_HEIGHT; y++) {
                sprite_0_hit_buffer[y][x] = false;
            }
        }

        UpdateTexture(texture, framebuffer);

        DrawTexturePro(texture, (Rectangle){0, 0, 256, 240},
                       (Rectangle){0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT},
                       (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        if (IsKeyPressed(KEY_TAB)) {
            show_debug = !show_debug;
        }

        if (show_debug)
            cpp_imGui_render(get_cpu_handle(), this);

        DrawFPS(0, 0);
        this->vblank = true;

        get_cpu_handle()->remaining_cycles += CYCLES_PER_LINE * 21;
        EndDrawing();
    }

    cpp_end();

    UnloadTexture(texture);
    free(framebuffer);

    CloseWindow();
    exit(0);
    return NULL;
}

uint8_t fetch_palette_index(uint16_t x, uint16_t y) {
    if (x < VIEWPORT_WIDTH && y < VIEWPORT_HEIGHT)
        return fetch_palette_index_with_nametable(x, y, 0);
    if (x >= VIEWPORT_WIDTH && y < VIEWPORT_HEIGHT)
        return fetch_palette_index_with_nametable(x % VIEWPORT_WIDTH, y, 1);
    if (x < VIEWPORT_WIDTH && y >= VIEWPORT_HEIGHT)
        return fetch_palette_index_with_nametable(x, y % VIEWPORT_HEIGHT, 2);

    return fetch_palette_index_with_nametable(x % VIEWPORT_WIDTH,
                                              y % VIEWPORT_HEIGHT, 3);
}

// x 0-512, y 0-480
uint8_t *fetch_background_pattern(uint16_t x, uint16_t y) {
    if (x < VIEWPORT_WIDTH && y < VIEWPORT_HEIGHT)
        return fetch_background_pattern_with_nametable(x / 8, y / 8, 0);
    if (x >= VIEWPORT_WIDTH && y < VIEWPORT_HEIGHT)
        return fetch_background_pattern_with_nametable((x % VIEWPORT_WIDTH) / 8,
                                                       y / 8, 1);
    if (x < VIEWPORT_WIDTH && y >= VIEWPORT_HEIGHT)
        return fetch_background_pattern_with_nametable(
            x / 8, (y % VIEWPORT_HEIGHT) / 8, 2);

    return fetch_background_pattern_with_nametable(
        (x % VIEWPORT_WIDTH) / 8, (y % VIEWPORT_HEIGHT) / 8, 3);
}

// x 0-256 y 0-240
uint8_t fetch_palette_index_with_nametable(uint8_t x, uint8_t y,
                                           uint8_t nametable_idx) {
    uint16_t base;
    switch (nametable_idx) {
    case 0:
        base = 0x23c0;
        break;
    case 1:
        base = 0x27c0;
        break;
    case 2:
        base = 0x2bc0;
        break;
    case 3:
        base = 0x2fc0;
        break;
    }
    uint8_t attrib = this->memory.cart.read(base + (x / 32) + (y / 32) * 8);

    if (x % 32 < 16 && y % 32 < 16) {
        return attrib & 0b11;
    }
    if (x % 32 >= 16 && y % 32 < 16) {
        return (attrib >> 2) & 0b11;
    }
    if (x % 32 < 16 && y % 32 >= 16) {
        return (attrib >> 4) & 0b11;
    }
    return attrib >> 6;
}

// x 0-32, y 0-30
uint8_t *fetch_background_pattern_with_nametable(uint8_t x, uint8_t y,
                                                 uint8_t nametable_idx) {
    uint16_t base;
    switch (nametable_idx) {
    case 0:
        base = 0x2000;
        break;
    case 1:
        base = 0x2400;
        break;
    case 2:
        base = 0x2800;
        break;
    case 3:
        base = 0x2c00;
        break;
    }

    return (uint8_t *)
        pattern_buffer[(this->background_pattern_table_address ? 256 : 0) +
                       this->memory.cart.read(base + x + 32 * y)];
}

void fetch_tile(uint16_t idx) {
    for (uint32_t i = 0; i < 8; i++) {
        uint8_t lower = this->memory.cart.read(idx * 16 + i);
        uint8_t higher = this->memory.cart.read(idx * 16 + i + 8);
        pattern_buffer[idx][i % 8][0] =
            (((lower & 0x80) != 0) ? 1 : 0) + (((higher & 0x80) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][1] =
            (((lower & 0x40) != 0) ? 1 : 0) + (((higher & 0x40) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][2] =
            (((lower & 0x20) != 0) ? 1 : 0) + (((higher & 0x20) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][3] =
            (((lower & 0x10) != 0) ? 1 : 0) + (((higher & 0x10) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][4] =
            (((lower & 0x08) != 0) ? 1 : 0) + (((higher & 0x08) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][5] =
            (((lower & 0x04) != 0) ? 1 : 0) + (((higher & 0x04) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][6] =
            (((lower & 0x02) != 0) ? 1 : 0) + (((higher & 0x02) != 0) ? 2 : 0);
        pattern_buffer[idx][i % 8][7] =
            (((lower & 0x01) != 0) ? 1 : 0) + (((higher & 0x01) != 0) ? 2 : 0);
    }
}
