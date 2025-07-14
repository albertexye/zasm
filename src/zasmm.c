/**
 * @file zasmm.c
 * @brief Microcode translation, mapping, and output generation for ZASM.
 *
 * This file implements the translation of assembler instructions to microcode signals,
 * mapping of microcode to hardware pin layouts, and generation of microcode output.
 * It also provides a command-line interface for generating microcode output files.
 *
 * Static helper functions are declared and documented at the top of the file.
 * Exported functions are documented in the corresponding header file.
 */

#include <assert.h>
#include <stddef.h>

#include "stream.h"
#include "zasm.h"
#include "zasmd.h"
#include "zasmm.h"

/**
 * @brief Set the output signals in the microcode for a given register.
 *
 * Handles special cases for certain registers (e.g., D and S) and sets the
 * appropriate output and ALU flags in the microcode structure.
 *
 * @param code Pointer to the microcode structure to modify.
 * @param r Register to set output for.
 */
static void ZM_setOut(ZM_Code_T *const code, const ZA_Reg_E r);

/**
 * @brief Set a pin in the layout structure as active.
 *
 * Sets the bit corresponding to the given pin number in the layout's pin array.
 *
 * @param layout Pointer to the layout structure to modify.
 * @param pin Pin number to set as active.
 */
static void setPin(ZM_Layout_T *const layout, const uint8_t pin);

const uint8_t ZM_OUT_PINS[13] = {
  3, 8, 7, 6, 0, 2, 0, 10, 5, 9, 4, 1, 11
};
const uint8_t ZM_IN_PINS[8] = {
  14, 17, 16, 15, 12, 13, 19, 18
};
const uint8_t ZM_SB_PIN = 20;
const uint8_t ZM_CN_PIN = 21;
const uint8_t ZM_HT_PIN = 22;

static void ZM_setOut(ZM_Code_T *const code, const ZA_Reg_E r) {
  // Ignore NOP and Z registers
  if (r == ZA_REG_N || r == ZA_REG_Z)
    return;
  if (r == ZA_REG_D) {
    // Special case: D register outputs to S and triggers ALU
    code->out[ZA_REG_S] = true;
    code->sb = true;
  } else {
    code->out[r] = true;
  }
}

ZM_Code_T ZM_translate(const ZA_Inst_T inst) {
  ZM_Code_T code = {0};
  switch (inst.op) {
  case ZA_OP_MOV:
    // MOV: Move value from r1 to r2 if not the same register
    if (inst.val.mov.r1 == inst.val.mov.r2) break;
    code.in[inst.val.mov.r1] = true;
    ZM_setOut(&code, inst.val.mov.r2);
    break;
  case ZA_OP_LDI:
    // LDI: Load immediate value into register
    code.in[inst.val.ldi.r] = true;
    code.out[12] = true;
    break;
  case ZA_OP_JEZ:
    // JEZ: Jump if equal to zero
    code.cn = true;
    [[fallthrough]];
  case ZA_OP_JNZ:
    // JNZ: Jump if not zero
    code.in[ZA_REG_P] = true;
    ZM_setOut(&code, inst.val.mov.r2);
    break;
  case ZA_OP_JNI:
    // JNI: Jump if negative immediate
    code.in[ZA_REG_P] = true;
    code.out[12] = true;
    break;
  case ZA_OP_HLT:
    // HLT: Halt
    code.ht = true;
    break;
  case ZA_OP_RST:
    // RST: Reset
    for (uint8_t i = 0; i < sizeof(code.in) / sizeof(code.in[0]); ++i) {
      code.in[i] = true;
    }
    break;
  default:
    assert(false);
  }
  return code;
}

ZM_Code_T ZM_activeLow(ZM_Code_T code) {
  constexpr size_t nOut = sizeof(code.out) / sizeof(code.out[0]);
  // Invert all output signals for active-low logic
  for (size_t i = 0; i < nOut; ++i) code.out[i] = !code.out[i];
  code.in[ZA_REG_M] = !code.in[ZA_REG_M];
  return code;
}

static void setPin(ZM_Layout_T *const layout, const uint8_t pin) {
  // Set the bit for the given pin in the layout
  layout->pins[pin / 8] |= (uint8_t)(1 << (pin % 8));
}

ZM_Layout_T ZM_map(const ZM_Code_T code) {
  ZM_Layout_T layout = {0};
  // Map output signals to pins
  for (size_t i = 0; i < sizeof(code.out) / sizeof(code.out[0]); ++i) {
    if (i == ZA_REG_N || !code.out[i]) continue;
    setPin(&layout, ZM_OUT_PINS[i]);
  }
  // Map input signals to pins
  for (size_t i = 0; i < sizeof(code.in) / sizeof(code.in[0]); ++i) {
    if (!code.in[i]) continue;
    setPin(&layout, ZM_IN_PINS[i]);
  }
  // Map control signals to pins
  if (code.sb) setPin(&layout, ZM_SB_PIN);
  if (code.cn) setPin(&layout, ZM_CN_PIN);
  if (code.ht) setPin(&layout, ZM_HT_PIN);
  return layout;
}

ZM_Layout_T ZM_macrocode(const uint8_t code) {
  // Parse instruction, apply active-low logic, and map to layout
  const ZA_Inst_T inst = ZD_parse(code);
  const ZM_Code_T mc = ZM_activeLow(ZM_translate(inst));
  return ZM_map(mc);
}

void ZM_generate(STM_Stream_T *const out, const uint8_t page, STM_Err_T *const err) {
  // Generate pin layouts for all 256 instruction codes
  for (uint16_t i = 0; i < 256; ++i) {
    const ZM_Layout_T layout = ZM_macrocode((uint8_t)i);
    STM_put(out, layout.pins[page], err);
    if (err->err != STM_ERR_OK) return;
  }
}

#if ZA_TGT == ZA_TGT_M
#include "zasmcli.h"

int main(const int argc, char *const argv[]) {
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_OUT, "out", {.stream = {0}}},
    {ZCLI_ARG_CHAR, "page", {.c = 0}},
  };
  ZCLI_ArgList_T arglist = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&arglist, argc, argv);
  STM_Err_T err = {0};
  const char page = args[1].value.c;
  if (page < '0' || page > '2') {
    ZCLI_error("page must be 0, 1, or 2");
    ZCLI_freeArgs(&arglist);
    return 1;
  }
  ZM_generate(&args[0].value.stream, (uint8_t)page - '0', &err);
  ZCLI_freeArgs(&arglist);
  if (err.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  return 0;
}
#endif
