#include <assert.h>
#include <stddef.h>

#include "stream.h"
#include "zasm.h"
#include "zasmd.h"
#include "zasmm.h"

const uint8_t ZM_OUT_PINS[12] = {0};
const uint8_t ZM_IN_PINS[8] = {0};
const uint8_t ZM_ALU_PIN = 0;
const uint8_t ZM_JMP_PIN = 0;
const uint8_t ZM_RST_PIN = 0;
const uint8_t ZM_HLT_PIN = 0;
const uint8_t ZM_INS_PIN = 0;

static void ZM_setOut(ZM_Code_T *const code, const ZA_Reg_E r) {
  if (r == ZA_REG_N || r == ZA_REG_Z)
    return;
  if (r == ZA_REG_D) {
    code->out[ZA_REG_S] = true;
    code->alu = true;
  } else {
    code->out[r] = true;
  }
}

ZM_Code_T ZM_translate(const ZA_Inst_T inst) {
  ZM_Code_T code = {0};
  switch (inst.op) {
  case ZA_OP_MOV:
    if (inst.val.mov.r1 == inst.val.mov.r2) break;
    code.in[inst.val.mov.r1] = true;
    ZM_setOut(&code, inst.val.mov.r2);
    break;
  case ZA_OP_LDI:
    code.in[inst.val.ldi.r] = true;
    code.ins = true;
    break;
  case ZA_OP_JEZ:
    code.jmp = true;
    [[fallthrough]];
  case ZA_OP_JNZ:
    code.in[ZA_REG_P] = true;
    ZM_setOut(&code, inst.val.mov.r2);
    break;
  case ZA_OP_JNI:
    code.ins = true;
    code.in[ZA_REG_P] = true;
    break;
  case ZA_OP_HLT:
    code.hlt = true;
    break;
  case ZA_OP_RST:
    code.rst = true;
    break;
  default:
    assert(false);
  }
  return code;
}

ZM_Code_T ZM_activeLow(ZM_Code_T code) {
  constexpr size_t nOut = sizeof(code.out) / sizeof(code.out[0]);
  for (size_t i = 0; i < nOut; ++i) code.out[i] = !code.out[i];
  code.rst = !code.rst;
  code.ins = !code.ins;
  return code;
}

static void setPin(ZM_Layout_T *const layout, const uint8_t pin) {
  layout->pins[pin / 8] |= (uint8_t)(1 << (pin % 8));
}

ZM_Layout_T ZM_map(const ZM_Code_T code) {
  ZM_Layout_T layout = {0};
  for (size_t i = 0; i < sizeof(code.out) / sizeof(code.out[0]); ++i) {
    if (!code.out[i]) continue;
    setPin(&layout, ZM_OUT_PINS[i]);
  }
  for (size_t i = 0; i < sizeof(code.in) / sizeof(code.in[0]); ++i) {
    if (!code.in[i]) continue;
    setPin(&layout, ZM_IN_PINS[i]);
  }
  if (code.alu) setPin(&layout, ZM_ALU_PIN);
  if (code.jmp) setPin(&layout, ZM_JMP_PIN);
  if (code.rst) setPin(&layout, ZM_RST_PIN);
  if (code.hlt) setPin(&layout, ZM_HLT_PIN);
  if (code.ins) setPin(&layout, ZM_INS_PIN);
  return layout;
}

ZM_Layout_T ZM_macrocode(const uint8_t code) {
  const ZA_Inst_T inst = ZD_parse(code);
  const ZM_Code_T mc = ZM_activeLow(ZM_translate(inst));
  return ZM_map(mc);
}

void ZM_generate(STM_Stream_T *const out, STM_Err_T *const err) {
  for (uint16_t i = 0; i < 256; ++i) {
    const ZM_Layout_T layout = ZM_macrocode((uint8_t)i);
    STM_write(out, layout.pins, sizeof(layout.pins), err);
    if (err->err != STM_ERR_OK) return;
  }
}

#if ZA_TGT == ZA_TGT_M
#include "zasmcli.h"

static const char *const USAGE = "zasmm [out]";

int main(const int argc, char *const argv[]) {
  if (argc != 2) {
    ZCLI_error("bad arguments");
    ZCLI_usage(USAGE);
    return 1;
  }
  STM_Stream_T out = ZCLI_openfile(argv[1], false);
  STM_Err_T err = {0};
  ZM_generate(&out, &err);
  ZCLI_closefile(&out);
  if (err.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  return 0;
}
#endif
