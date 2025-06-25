#include <assert.h>

#include "zasm.h"
#include "stream.h"

const char g_ZA_OpNames[7][4] = {
  "mov",
  "ldi",
  "jez",
  "jnz",
  "jni",
  "hlt",
  "rst",
};

const char g_ZA_RegNames[14] = "acgmxynpbdjlsz";

bool ZA_checkInst(const ZA_Inst_T inst) {
  switch (inst.op) {
    case ZA_OP_MOV:
      return inst.val.mov.r1 <= ZA_REG_N && inst.val.mov.r2 <= ZA_REG_Z;
    case ZA_OP_LDI:
      return inst.val.ldi.r <= ZA_REG_N;
    case ZA_OP_JEZ:
    case ZA_OP_JNZ:
      return inst.val.jmp <= ZA_REG_Z;
    case ZA_OP_JNI:
    case ZA_OP_HLT:
    case ZA_OP_RST:
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
  if (!ZA_checkInst(inst)) {
    STM_printf(out, err, "invalid instruction");
    return;
  }
  switch (inst.op) {
    case ZA_OP_MOV:
      STM_printf(out, err, "r%c > r%c", g_ZA_RegNames[inst.val.mov.r2], g_ZA_RegNames[inst.val.mov.r1]);
      break;
    case ZA_OP_LDI:
      STM_printf(out, err, "%hhu > r%c", inst.val.ldi.i, g_ZA_RegNames[inst.val.ldi.r]);
      break;
    case ZA_OP_JEZ:
      STM_printf(out, err, "!-> r%c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNZ:
      STM_printf(out, err, "-> r%c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNI:
      STM_printf(out, err, "!-> %hhu", inst.val.imm);
      break;
    case ZA_OP_HLT:
      STM_printf(out, err, "halt");
      break;
    case ZA_OP_RST:
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
  if (!ZA_checkInst(inst)) {
    STM_printf(out, err, "; invalid instruction");
    return;
  }
  STM_printf(out, err, g_ZA_OpNames[inst.op]);
  if (err->err != STM_ERR_OK) return;
  switch (inst.op) {
    case ZA_OP_MOV:
      STM_printf(out, err, " %c %c", g_ZA_RegNames[inst.val.mov.r1], g_ZA_RegNames[inst.val.mov.r2]);
      break;
    case ZA_OP_LDI:
      STM_printf(out, err, " %c %hhu", g_ZA_RegNames[inst.val.ldi.r], inst.val.ldi.i);
      break;
    case ZA_OP_JEZ:
    case ZA_OP_JNZ:
      STM_printf(out, err, " %c", g_ZA_RegNames[inst.val.jmp]);
      break;
    case ZA_OP_JNI:
      STM_printf(out, err, " %hhu", inst.val.imm);
      break;
    case ZA_OP_HLT:
    case ZA_OP_RST:
      break;
    default:
      assert(false);
  }
}

