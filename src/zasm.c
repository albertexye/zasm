/**
 * @file zasm.c
 * @brief Implementation of ZASM instruction utilities and string tables.
 *
 * This file provides the implementation for ZASM instruction validation, explanation, and printing.
 */

#include <assert.h>

#include "zasm.h"
#include "stream.h"

/**
 * @brief String names for each operation code (indexed by ZA_Op_E).
 */
const char g_ZA_OpNames[7][4] = {
  "mov",
  "ldi",
  "jez",
  "jnz",
  "jni",
  "hlt",
  "rst",
};

/**
 * @brief String names for each register (indexed by ZA_Reg_E).
 */
const char g_ZA_RegNames[14] = "acgmxynpbjlsdz";

bool ZA_checkInst(const ZA_Inst_T inst) {
  // Validate instruction based on opcode and argument ranges
  switch (inst.op) {
    case ZA_OP_MOV:
      // MOV: r1 and r2 must be valid registers
      return inst.val.mov.r1 <= ZA_REG_N && inst.val.mov.r2 <= ZA_REG_Z;
    case ZA_OP_LDI:
      // LDI: r must be a valid register
      return inst.val.ldi.r <= ZA_REG_N;
    case ZA_OP_JEZ:
    case ZA_OP_JNZ:
      // JEZ/JNZ: jmp must be a valid register
      return inst.val.jmp <= ZA_REG_Z;
    case ZA_OP_JNI:
    case ZA_OP_HLT:
    case ZA_OP_RST:
      // JNI/HLT/RST: always valid
      return true;
    default:
      return false;
  }
}

void ZA_explainInst(
  const ZA_Inst_T inst,
  STM_Stream_T* const out,
  STM_Err_T* const err
) {
  // Print a human-readable explanation of the instruction
  if (!ZA_checkInst(inst)) {
    STM_printf(out, err, "invalid instruction");
    return;
  }
  switch (inst.op) {
    case ZA_OP_MOV:
      // MOV: r2 > r1
      STM_printf(out, err, "r%c > r%c", g_ZA_RegNames[inst.val.mov.r2], g_ZA_RegNames[inst.val.mov.r1]);
      break;
    case ZA_OP_LDI:
      // LDI: immediate > r
      STM_printf(out, err, "%hhu > r%c", inst.val.ldi.i, g_ZA_RegNames[inst.val.ldi.r]);
      break;
    case ZA_OP_JEZ:
      // JEZ: jump if zero
      STM_printf(out, err, "!-> r%c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNZ:
      // JNZ: jump if not zero
      STM_printf(out, err, "-> r%c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNI:
      // JNI: jump to immediate
      STM_printf(out, err, "!-> %hhu", inst.val.imm);
      break;
    case ZA_OP_HLT:
      // HLT: halt
      STM_printf(out, err, "halt");
      break;
    case ZA_OP_RST:
      // RST: reset
      STM_printf(out, err, "reset");
      break;
    default:
      assert(false);
  }
}

void ZA_printInst(
  const ZA_Inst_T inst,
  STM_Stream_T* const out,
  STM_Err_T* const err
) {
  // Print the instruction in assembly format
  if (!ZA_checkInst(inst)) {
    STM_printf(out, err, "; invalid instruction");
    return;
  }
  STM_printf(out, err, g_ZA_OpNames[inst.op]);
  if (err->err != STM_ERR_OK) return;
  switch (inst.op) {
    case ZA_OP_MOV:
      // MOV: print r1 and r2
      STM_printf(out, err, " %c %c", g_ZA_RegNames[inst.val.mov.r1], g_ZA_RegNames[inst.val.mov.r2]);
      break;
    case ZA_OP_LDI:
      // LDI: print r and immediate
      STM_printf(out, err, " %c %hhu", g_ZA_RegNames[inst.val.ldi.r], inst.val.ldi.i);
      break;
    case ZA_OP_JEZ:
    case ZA_OP_JNZ:
      // JEZ/JNZ: print jump register
      STM_printf(out, err, " %c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNI:
      // JNI: print immediate
      STM_printf(out, err, " %hhu", inst.val.imm);
      break;
    case ZA_OP_HLT:
    case ZA_OP_RST:
      // HLT/RST: no arguments
      break;
    default:
      assert(false);
  }
}

