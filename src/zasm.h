#pragma once

#include <stdint.h>

#include "stream.h"

#define ZA_TGT_C 1
#define ZA_TGT_D 2
#define ZA_TGT_S 3
#define ZA_TGT_M 4
#define ZA_TGT_F 5

#ifndef ZA_TGT
#define ZA_TGT 0
#endif

typedef enum {
  ZA_OP_MOV,
  ZA_OP_LDI,
  ZA_OP_JEZ,
  ZA_OP_JNZ,
  ZA_OP_JNI,
  ZA_OP_HLT,
  ZA_OP_RST,
} ZA_Op_E;

extern const char g_ZA_OpNames[7][4];

typedef enum {
  // RW
  ZA_REG_A, // Address
  ZA_REG_C, // Conditional
  ZA_REG_G, // General Purpose
  ZA_REG_M, // Memory
  ZA_REG_X, // Operand 1
  ZA_REG_Y, // Operand 2
  // WO
  ZA_REG_N, // Number
  // RO
  ZA_REG_P, // Program Counter
  ZA_REG_B, // Buttons
  ZA_REG_J, // Jump
  ZA_REG_L, // Left Shift
  ZA_REG_S, // Sum
  ZA_REG_D, // Difference
  ZA_REG_Z, // Zero
} ZA_Reg_E;

extern const char g_ZA_RegNames[14];

typedef struct {
  ZA_Reg_E r1, r2;
} ZA_MovArgs_T;

typedef struct {
  ZA_Reg_E r;
  uint8_t i: 4;
} ZA_LdiArgs_T;

typedef union {
  ZA_MovArgs_T mov;
  ZA_LdiArgs_T ldi;
  ZA_Reg_E jmp;
  uint8_t imm: 4;
} ZA_Args_T;

typedef struct {
  ZA_Op_E op;
  ZA_Args_T val;
} ZA_Inst_T;

bool ZA_checkInst(ZA_Inst_T inst);

void ZA_explainInst(ZA_Inst_T inst, STM_Stream_T* out, STM_Err_T* eof);

void ZA_printInst(ZA_Inst_T inst, STM_Stream_T* out, STM_Err_T* eof);

