#pragma once

#include <stdint.h>

typedef struct {
  uint8_t mem[256];
  uint8_t rom[256];
  uint8_t r[7];
  uint8_t b;
  uint8_t pc;
  bool halted;
} ZS_State_T;

void ZS_new(ZS_State_T* state);

void ZS_exec(ZS_State_T* state);

