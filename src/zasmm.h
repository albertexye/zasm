#pragma once

#include <stdint.h>

#include "stream.h"
#include "zasm.h"

extern const uint8_t ZM_OUT_PINS[12];
extern const uint8_t ZM_IN_PINS[8];
extern const uint8_t ZM_ALU_PIN;
extern const uint8_t ZM_JMP_PIN;
extern const uint8_t ZM_RST_PIN;
extern const uint8_t ZM_HLT_PIN;
extern const uint8_t ZM_INS_PIN;

typedef struct {
  bool out[12];
  bool in[8];
  bool alu, jmp, rst, hlt, ins;
} ZM_Code_T;

typedef struct {
  uint8_t pins[3];
} ZM_Layout_T;

ZM_Code_T ZM_translate(ZA_Inst_T inst);

ZM_Code_T ZM_activeLow(ZM_Code_T code);

ZM_Layout_T ZM_map(ZM_Code_T code);

ZM_Layout_T ZM_macrocode(uint8_t code);

void ZM_generate(STM_Stream_T* out, STM_Err_T* eof);
