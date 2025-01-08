#include "apu.h"
#include "cpu.h"
#include "mappers/M000.h"
#include "mappers/M004.h"
#include "ppu.h"
#include "types.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *f = fopen("resources/smb.nes", "rb");
    int8_t header[16] = {0};
    fread(header, 16, 1, f);
    if (strncmp((char[]){'N', 'E', 'S', 0x1a}, (char *)header, 4) != 0) {
        printf("Invalid file header");
        exit(1);
    }

    uint8_t mapper = (header[6] >> 4) | (header[7] & 0xf);

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
        ppu.memory.cart.read = m000_ppu_read;
        ppu.memory.cart.write = m000_ppu_write;
        break;
    case 0x04:
        m004_init(f, header[4], header[5], header[6] & 1, header[6] & 0x80);
        cpu.memory.cart.read = m004_cpu_read;
        cpu.memory.cart.write = m004_cpu_write;
        ppu.memory.cart.read = m004_ppu_read;
        ppu.memory.cart.write = m004_ppu_write;
        break;
    default:
        printf("Unsupported mapper %d\n", mapper);
        exit(1);
        break;
    }
    pthread_t ui_thread_handle;
    pthread_create(&ui_thread_handle, NULL, ui_thread, NULL);

    cpu_init(&cpu);
    cpu.ppu = &ppu;
    cpu.apu = &apu;
    ppu_init(&ppu);
    apu_init(&apu);
    interrupt(RESET);

    pthread_join(ui_thread_handle, NULL);
    return 0;
}
