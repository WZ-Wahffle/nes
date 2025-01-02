#pragma once

#include "../types.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

  EXTERNC void cpp_init(void);

  EXTERNC void cpp_imGui_render(cpu_t* cpu, ppu_t* ppu);

  EXTERNC void cpp_end(void);
