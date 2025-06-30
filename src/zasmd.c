/**
 * @file zasmd.c
 * @brief Implementation of the ZASM disassembler logic.
 *
 * This file contains the implementation of the ZASM disassembler, including
 * static helper functions for parsing instruction codes and the main disassembly logic.
 *
 * Static functions are documented here for maintainers. Exported functions are documented in the header.
 *
 * Example usage:
 *   See main() for a command-line disassembler example (enabled with ZA_TGT == ZA_TGT_D).
 */

#include <stdint.h>

#include "stream.h"
#include "zasm.h"
#include "zasmd.h"

/**
 * @brief Parse an immediate or jump instruction from a code byte.
 *
 * Decodes the given 8-bit code as either a JNI (jump if negative immediate)
 * or LDI (load immediate) instruction, depending on the upper nibble.
 *
 * @param code The 8-bit instruction code to parse.
 * @return ZA_Inst_T The parsed instruction structure.
 *
 * @note Only used internally by the disassembler.
 */
static ZA_Inst_T parseI(uint8_t code);

/**
 * @brief Parse a MOV or JNZ instruction from a code byte.
 *
 * Decodes the given 8-bit code as either a MOV (move register) or
 * JNZ (jump if not zero) instruction, depending on the upper nibble.
 *
 * @param code The 8-bit instruction code to parse.
 * @return ZA_Inst_T The parsed instruction structure.
 *
 * @note Only used internally by the disassembler.
 */
static ZA_Inst_T parseM(uint8_t code);

static ZA_Inst_T parseI(const uint8_t code) {
  // Check if upper nibble is 0b1111 for JNI, else LDI
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
  // If upper nibble is not 0b111, it's a MOV, else JNZ
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
  // Top bit set: parse as immediate/jump instruction
  if (code >> 7) return parseI(code);
  // Lower nibble < 0b1110: parse as MOV/JNZ
  if ((code & 0b1111) < 0b1110) return parseM(code);
  // Special cases for HLT and RST
  if (code == 0b01101111) return (ZA_Inst_T) {
    .op = ZA_OP_HLT,
  };
  if (code == 0b01111111) return (ZA_Inst_T) {
    .op = ZA_OP_RST,
  };
  // Default: parse as JEZ
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
    // Read next instruction code from input stream
    const uint8_t code = STM_get(in, err);
    if (err->err != STM_ERR_OK) {
      // End of file or error: exit loop
      if (err->err == STM_ERR_EOF)
        err->err = STM_ERR_OK;
      return;
    }
    // Parse the instruction
    const ZA_Inst_T inst = ZD_parse(code);
    // Print the disassembled instruction
    ZA_printInst(inst, out, err);
    if (err->err != STM_ERR_OK) return;
    // Write newline after each instruction
    STM_put(out, '\n', err);
    if (err->err != STM_ERR_OK) return;
  }
}

#if ZA_TGT == ZA_TGT_D
#include "zasmcli.h"

int main(const int argc, char* const argv[]) {
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_IN, "bin", {.stream = {0}}},
    {ZCLI_ARG_STREAM_OUT, "out", {.stream = {0}}},
  };
  ZCLI_ArgList_T arglist = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&arglist, argc, argv);
  STM_Err_T err = {0};
  ZD_disassemble(&args[0].value.stream, &args[1].value.stream, &err);
  ZCLI_freeArgs(&arglist);
  if (err.err != STM_ERR_OK) {
    ZCLI_error("failed to write file");
    return 1;
  }
  return 0;
}
#endif
