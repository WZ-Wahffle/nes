#include "apu.h"
#include "cpu.h"
#include "mappers/M000.h"
#include "mappers/M001.h"
#include "mappers/M002.h"
#include "mappers/M004.h"
#include "ppu.h"
#include "types.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void exit_callback(void) {
    cpu_t *cpu = get_cpu_handle();
    printf("Stack trace: \n");
    for (uint32_t i = cpu->prev_inst_idx; i != cpu->prev_inst_idx - 1; i++) {
        if (i == 0x1000)
            i = 0;
        printf("0x%02x at 0x%04x, bank %d\n", cpu->prev_inst[i],
               cpu->prev_pc[i], cpu->prev_bank[i]);
    }
}

int main(void) {
    FILE *f = fopen("resources/tloz.nes", "rb");
    int8_t header[16] = {0};
    fread(header, 16, 1, f);
    if (strncmp((char[]){'N', 'E', 'S', 0x1a}, (char *)header, 4) != 0) {
        printf("Invalid file header");
        exit(1);
    }

    uint8_t mapper = (header[6] >> 4) | (header[7] & 0xf0);

    if (header[6] & 0x4) {
        printf("ROM trailer not supported\n");
        exit(1);
    }

    cpu_t cpu = {0};
    ppu_t ppu = {0};
    apu_t apu = {0};

    switch (mapper) {
    case 0x00:
        m000_init(f, header[4], header[8], header[6] & 1);
        cpu.memory.cart.read = m000_cpu_read;
        cpu.memory.cart.write = m000_cpu_write;
        cpu.cpu_addr_to_bank_callback = m000_get_bank_from_cpu_addr;
        ppu.memory.cart.read = m000_ppu_read;
        ppu.memory.cart.write = m000_ppu_write;
        break;
    case 0x01:
        m001_init(f, header[4], header[5]);
        cpu.memory.cart.read = m001_cpu_read;
        cpu.memory.cart.write = m001_cpu_write;
        cpu.cpu_addr_to_bank_callback = m001_get_bank_from_cpu_addr;
        ppu.memory.cart.read = m001_ppu_read;
        ppu.memory.cart.write = m001_ppu_write;
        break;
    case 0x02:
        m002_init(f, header[4], header[6] & 1);
        cpu.memory.cart.read = m002_cpu_read;
        cpu.memory.cart.write = m002_cpu_write;
        cpu.cpu_addr_to_bank_callback = m000_get_bank_from_cpu_addr;
        ppu.memory.cart.read = m002_ppu_read;
        ppu.memory.cart.write = m002_ppu_write;
        break;
    case 0x04:
        m004_init(f, header[4], header[5], header[6] & 1, header[6] & 0x80);
        cpu.memory.cart.read = m004_cpu_read;
        cpu.memory.cart.write = m004_cpu_write;
        cpu.cpu_addr_to_bank_callback = m004_get_bank_from_cpu_addr;
        ppu.memory.cart.read = m004_ppu_read;
        ppu.memory.cart.write = m004_ppu_write;
        ppu.end_of_scanline_callback = m004_scanline_callback;
        break;
    default:
        printf("Unsupported mapper %d\n", mapper);
        exit(1);
        break;
    }
    pthread_t ui_thread_handle;
    pthread_create(&ui_thread_handle, NULL, ui_thread, NULL);

    atexit(exit_callback);

    cpu_init(&cpu);
    cpu.ppu = &ppu;
    cpu.apu = &apu;
    ppu_init(&ppu);
    apu_init(&apu);
    interrupt(RESET);

    pthread_join(ui_thread_handle, NULL);
    return 0;
}
