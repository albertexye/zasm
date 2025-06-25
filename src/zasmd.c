#include <stdint.h>

#include "stream.h"
#include "zasm.h"
#include "zasmd.h"

static ZA_Inst_T parseI(const uint8_t code) {
  if ((code >> 4) == 0b1111) return (ZA_Inst_T) {
    .op = ZA_OP_JNI,
    .val.imm = code & 0b1111,
  };
  return (ZA_Inst_T) {
    .op = ZA_OP_LDI,
    .val.ldi = {
      .r = (code >> 4) & 0b111,
      .i = code & 0b1111,
    },
  };
}

static ZA_Inst_T parseM(const uint8_t code) {
  if ((code >> 4) != 0b111) return (ZA_Inst_T) {
    .op = ZA_OP_MOV,
    .val.mov = {
      .r1 = code >> 4,
      .r2 = code & 0b1111,
    },
  };
  return (ZA_Inst_T) {
    .op = ZA_OP_JNZ,
    .val.jmp = code & 0b1111,
  };
}

ZA_Inst_T ZD_parse(const uint8_t code) {
  if (code >> 7) return parseI(code);
  if ((code & 0b1111) < 0b1110) return parseM(code);
  if (code == 0b01101111) return (ZA_Inst_T) {
    .op = ZA_OP_HLT,
  };
  if (code == 0b01111111) return (ZA_Inst_T) {
    .op = ZA_OP_RST,
  };
  return (ZA_Inst_T) {
    .op = ZA_OP_JEZ,
    .val.jmp = (uint8_t)(((code & 1) << 3) | (code >> 4)),
  };
}

void ZD_disassemble(
  STM_Stream_T* const in,
  STM_Stream_T* const out,
  STM_Err_T* const err
) {
  while (true) {
    const uint8_t code = STM_get(in, err);
    if (err->err != STM_ERR_OK) {
      if (err->err == STM_ERR_EOF)
        err->err = STM_ERR_OK;
      return;
    }
    const ZA_Inst_T inst = ZD_parse(code);
    ZA_printInst(inst, out, err);
    if (err->err != STM_ERR_OK) return;
    STM_put(out, '\n', err);
    if (err->err != STM_ERR_OK) return;
  }
}

#if ZA_TGT == ZA_TGT_D
#include "zasmcli.h"

static const char* const USAGE = "zasmd bin out";

int main(const int argc, char* const argv[]) {
  if (argc != 3) {
    ZCLI_error("bad arguments");
    ZCLI_usage(USAGE);
    return 1;
  }
  STM_Stream_T in = ZCLI_openfile(argv[1], true);
  STM_Stream_T out = ZCLI_openfile(argv[2], false);
  STM_Err_T err = {0};
  ZD_disassemble(&in, &out, &err);
  ZCLI_closefile(&in);
  ZCLI_closefile(&out);
  if (err.err != STM_ERR_OK) {
    ZCLI_error("failed to write file");
    return 1;
  }
  return 0;
}
#endif

