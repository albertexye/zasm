#pragma once

#include <stdint.h>

#include "stream.h"
#include "zasm.h"

ZA_Inst_T ZD_parse(uint8_t code);

void ZD_disassemble(STM_Stream_T* in, STM_Stream_T* out, STM_Err_T* err);

