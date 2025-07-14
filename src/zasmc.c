/**
 * @file zasmc.c
 * @brief ZASM Compiler core implementation.
 *
 * This file implements the core logic for the ZASM assembler, including tokenization, parsing, code generation, and error handling.
 *
 * Only static (internal) functions are documented here. Exported functions are documented in the header file.
 *
 * Notes:
 *   - All static functions are declared at the top of the file with Doxygen documentation.
 *   - Inline comments are provided in function bodies to clarify logic where necessary.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stream.h"
#include "zasm.h"
#include "zasmc.h"

/**
 * @brief Converts a character to lowercase if it is uppercase.
 * @param c Input character.
 * @return Lowercase character if input is uppercase, otherwise unchanged.
 */
static char ZC_toLower(const char c);

/**
 * @brief Parses an operation token into an operation enum value.
 * @param token Operation token.
 * @param err Output error structure.
 * @return Parsed operation enum value.
 */
static ZA_Op_E ZC_parseOp(const ZC_Token_T token, ZC_Err_T* const err);

/**
 * @brief Parses a register token into a register enum value.
 * @param token Register token.
 * @param err Output error structure.
 * @return Parsed register enum value.
 */
static ZA_Reg_E ZC_parseReg(const ZC_Token_T token, ZC_Err_T* const err);

/**
 * @brief Parses a hexadecimal string into an 8-bit value.
 * @param s Hexadecimal string.
 * @param err Output error structure.
 * @return Parsed 8-bit value.
 */
static uint8_t ZC_parseHex(const char* const s, ZC_Err_T* const err);

/**
 * @brief Parses a binary string into an 8-bit value.
 * @param s Binary string.
 * @param err Output error structure.
 * @return Parsed 8-bit value.
 */
static uint8_t ZC_parseBinary(const char* const s, ZC_Err_T* const err);

/**
 * @brief Parses a decimal string into an 8-bit value.
 * @param s Decimal string.
 * @param err Output error structure.
 * @return Parsed 8-bit value.
 */
static uint8_t ZC_parseDecimal(const char* const s, ZC_Err_T* const err);

/**
 * @brief Parses an immediate token into an 8-bit value.
 * @param token Immediate token.
 * @param err Output error structure.
 * @return Parsed 8-bit value.
 */
static uint8_t ZC_parseImm(const ZC_Token_T token, ZC_Err_T* const err);

/**
 * @brief Checks if a token marks the end of a line or file.
 * @param token Token to check.
 * @return true if token is end-of-line or end-of-file, false otherwise.
 */
static bool ZC_tokenEnd(const ZC_Token_T token);

/**
 * @brief Determines the number of tokens in a line.
 * @param line Line to check.
 * @return Number of tokens in the line.
 */
static size_t ZC_lineSize(const ZC_Line_T line);

/**
 * @brief Checks if a character is considered whitespace.
 * @param c Character to check.
 * @return true if whitespace, false otherwise.
 */
static bool ZC_isWhitespace(const char c);

/**
 * @brief Clears the remainder of the current line in the stream.
 * @param stream Input stream.
 * @param err Output stream error structure.
 */
static void ZC_clearLine(STM_Stream_T* const stream, STM_Err_T* const err);

/**
 * @brief Reads a character from the stream and updates token state.
 * @param stream Input stream.
 * @param token Token to update.
 * @param c Output character.
 * @param err Output error structure.
 * @return true if end-of-line or end-of-file, false otherwise.
 */
static bool ZC_readChar(STM_Stream_T* const stream, ZC_Token_T* const token, char* const c, ZC_Err_T* const err);

/**
 * @brief Reads a token from the input stream.
 * @param stream Input stream.
 * @param err Output error structure.
 * @return Parsed token.
 */
static ZC_Token_T ZC_readToken(STM_Stream_T* const stream, ZC_Err_T* const err);

static const char* const s_ZC_ErrMsg[] = {
  "",
  "invalid operation",
  "invalid register",
  "invalid immediate base",
  "invalid immediate digit",
  "bad immediate len",
  "immediate overflow",
  "invalid character",
  "bad token len",
  "bad line len",
  "bad instruction format",
  "writing to read-only register",
};

static char ZC_toLower(const char c) {
  if (c >= 'A' && c <= 'Z') return c + 'a' - 'A';
  return c;
}

static ZA_Op_E ZC_parseOp(const ZC_Token_T token, ZC_Err_T* const err) {
  if (token.str[3] != 0) goto error;
  constexpr size_t len = sizeof(g_ZA_OpNames) / sizeof(g_ZA_OpNames[0]);
  for (size_t i = 0; i < len; ++i) {
    if (memcmp(token.str, g_ZA_OpNames[i], 3) == 0)
      return (ZA_Op_E)i;
  }
error:
  err->err = ZC_ERR_OP;
  return ZA_OP_MOV;
}

static ZA_Reg_E ZC_parseReg(const ZC_Token_T token, ZC_Err_T* const err) {
  if (token.str[1] != 0) goto error;
  constexpr size_t len = sizeof(g_ZA_RegNames) / sizeof(g_ZA_RegNames[0]);
  const char r = token.str[0];
  for (size_t i = 0; i < len; ++i) {
    if (r == g_ZA_RegNames[i]) return (ZA_Reg_E)i;
  }
error:
  err->err = ZC_ERR_REG;
  return ZA_REG_A;
}

static uint8_t ZC_parseHex(const char* const s, ZC_Err_T* const err) {
  if (s[1] != 0) {
    err->err = ZC_ERR_IMM_LEN;
    return 0;
  }
  const char c = s[0];
  if (c >= '0' && c <= '9') return (uint8_t)c - '0';
  if (c >= 'A' && c <= 'F') return (uint8_t)c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return (uint8_t)c - 'a' + 10;
  err->err = ZC_ERR_IMM_DIGIT;
  return 0;
}

static uint8_t ZC_parseBinary(const char* const s, ZC_Err_T* const err) {
  if (s[0] == 0 || s[4] != 0) {
    err->err = ZC_ERR_IMM_LEN;
    return 0;
  }
  uint8_t ret = 0;
  for (int i = 0; i < 4; ++i) {
    const char c = s[i];
    if (c == 0) return ret;
    if (c != '0' && c != '1') {
      err->err = ZC_ERR_IMM_DIGIT;
      return 0;
    }
    ret |= c - '0';
    ret <<= 1;
  }
  return ret;
}

static uint8_t ZC_parseDecimal(const char* const s, ZC_Err_T* const err) {
  if (s[0] < '1' || s[0] > '9') {
    err->err = ZC_ERR_IMM_DIGIT;
    return 0;
  }
  if (s[1] == 0) return (uint8_t)(s[0] - '0');
  if (s[1] < '0' || s[1] > '9') {
    err->err = ZC_ERR_IMM_DIGIT;
    return 0;
  }
  if (s[2] != 0) {
    err->err = ZC_ERR_IMM_LEN;
    return 0;
  }
  const uint8_t ret = (uint8_t)((s[0] - '0') * 10 + (s[1] - '0'));
  if (ret > 15) {
    err->err = ZC_ERR_IMM_OVERFLOW;
    return 0;
  }
  return ret;
}

static uint8_t ZC_parseImm(const ZC_Token_T token, ZC_Err_T* const err) {
  const char* const s = token.str;
  if (s[0] == '0') {
    if (s[1] == 'x') return ZC_parseHex(s + 2, err);
    if (s[1] == 'b') return ZC_parseBinary(s + 2, err);
    if (s[1] == 0) return 0;
    err->err = ZC_ERR_IMM_BASE;
    return 0;
  }
  return ZC_parseDecimal(s, err);
}

static bool ZC_tokenEnd(const ZC_Token_T token) {
  return token.eol || token.eof;
}

static size_t ZC_lineSize(const ZC_Line_T line) {
  constexpr size_t size = sizeof(line.tokens) / sizeof(line.tokens[0]);
  for (size_t i = 0; i < size; ++i) {
    if (ZC_tokenEnd(line.tokens[i])) return i + 1;
  }
  assert(false);
}

ZA_Inst_T ZC_parse(const ZC_Line_T line, ZC_Err_T* const err) {
  ZA_Inst_T inst = {0};
  inst.op = ZC_parseOp(line.tokens[0], err);
  if (err->err != ZC_ERR_OK) return inst;
  const size_t size = ZC_lineSize(line);
  switch (inst.op) {
    case ZA_OP_MOV:
      if (size != 3) break;
      inst.val.mov.r1 = ZC_parseReg(line.tokens[1], err);
      if (err->err != ZC_ERR_OK) return inst;
      inst.val.mov.r2 = ZC_parseReg(line.tokens[2], err);
      return inst;
    case ZA_OP_LDI:
      if (size != 3) break;
      inst.val.ldi.r = ZC_parseReg(line.tokens[1], err);
      if (err->err != ZC_ERR_OK) return inst;
      inst.val.ldi.i = ZC_parseImm(line.tokens[2], err);
      return inst;
    case ZA_OP_JEZ:
    case ZA_OP_JNZ:
      if (size != 2) break;
      inst.val.jmp = ZC_parseReg(line.tokens[1], err);
      return inst;
    case ZA_OP_JNI:
      if (size != 2) break;
      inst.val.imm = ZC_parseImm(line.tokens[1], err);
      return inst;
    case ZA_OP_HLT:
    case ZA_OP_RST:
      if (size != 1) break;
      return inst;
    default:
      assert(false);
  }
  err->err = ZC_ERR_INST_FMT;
  return inst;
}

static bool ZC_isWhitespace(const char c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\f':
    case '\v':
      return true;
    default:
      return false;
  }
}

static void ZC_clearLine(STM_Stream_T* const stream, STM_Err_T* const err) {
  while (STM_get(stream, err) != '\n')
    if (err->err != STM_ERR_OK) return;
}

static bool ZC_readChar(STM_Stream_T* const stream, ZC_Token_T* const token, char* const c, ZC_Err_T* const err) {
  STM_Err_T serr = {0};
  *c = (char)STM_get(stream, &serr);
  if (serr.err != STM_ERR_OK) {
    token->eof = true;
    token->eol = true;
    if (serr.err != STM_ERR_EOF) {
      err->err = ZC_ERR_STM;
      err->code.stm = serr;
    }
    return true;
  }
  if (*c < 0) {
    err->err = ZC_ERR_INVAL_CHAR;
    return true;
  }
  if (*c == '\n') {
    token->eol = true;
    return true;
  }
  if (*c == ';') {
    ZC_clearLine(stream, &serr);
    token->eol = true;
    if (serr.err == STM_ERR_EOF) {
      token->eof = true;
    } else if (serr.err != STM_ERR_OK) {
      token->eof = true;
      err->err = ZC_ERR_STM;
      err->code.stm = serr;
    }
    return true;
  }
  *c = ZC_toLower(*c);
  return false;
}

static ZC_Token_T ZC_readToken(STM_Stream_T* const stream, ZC_Err_T* const err) {
  ZC_Token_T token = {0};
  char c;
  while (true) {
    if (ZC_readChar(stream, &token, &c, err)) return token;
    if (!ZC_isWhitespace(c)) break;
  }
  token.str[0] = c;
  for (int i = 1; i < 6; ++i) {
    if (ZC_readChar(stream, &token, &c, err)) return token;
    if (ZC_isWhitespace(c)) return token;
    token.str[i] = c;
  }
  if (ZC_readChar(stream, &token, &c, err)) return token;
  if (ZC_isWhitespace(c)) return token;
  err->err = ZC_ERR_TOKEN_LEN;
  return token;
}

ZC_Line_T ZC_tokenize(STM_Stream_T* const stream, ZC_Err_T* const err) {
  ZC_Line_T line = {0};
  for (int i = 0; i < 3; ++i) {
    const ZC_Token_T token = ZC_readToken(stream, err);
    if (err->err != ZC_ERR_OK) return line;
    line.tokens[i] = token;
    if (ZC_tokenEnd(token)) {
      if (token.str[0] == 0 && i != 0) {
        line.tokens[i - 1].eol = token.eol;
        line.tokens[i - 1].eof = token.eof;
      }
      return line;
    }
  }
  const ZC_Token_T token = ZC_readToken(stream, err);
  if (err->err != ZC_ERR_OK) return line;
  if (ZC_tokenEnd(token) && token.str[0] == 0) {
    line.tokens[2].eol = token.eol;
    line.tokens[2].eof = token.eof;
    return line;
  }
  err->err = ZC_ERR_LINE_LEN;
  return line;
}

uint8_t ZC_generate(const ZA_Inst_T inst, ZC_Err_T* const err) {
  switch (inst.op) {
    case ZA_OP_MOV:
      if (inst.val.mov.r1 > ZA_REG_N) goto error;
      assert(inst.val.mov.r2 <= ZA_REG_Z);
      return (uint8_t)((inst.val.mov.r1 << 4) | inst.val.mov.r2);
    case ZA_OP_LDI:
      if (inst.val.ldi.r > ZA_REG_N) goto error;
      return (uint8_t)(0b10000000 | (inst.val.ldi.r << 4) | inst.val.ldi.i);
    case ZA_OP_JEZ:
      assert(inst.val.jmp <= ZA_REG_Z);
      return (uint8_t)(0b00001110 | (inst.val.jmp >> 3) | ((inst.val.jmp & 0b111) << 4));
    case ZA_OP_JNZ:
      assert(inst.val.jmp <= ZA_REG_Z);
      return (uint8_t)(0b01110000 | inst.val.jmp);
    case ZA_OP_JNI:
      return (uint8_t)(0b11110000 | inst.val.imm);
    case ZA_OP_HLT:
      return 0b01101111;
    case ZA_OP_RST:
      return 0b01111111;
    default:
      assert(false);
  }
error:
  err->err = ZC_ERR_RO_REG;
  return 0;
}

size_t ZC_compile(
  STM_Stream_T* const in,
  STM_Stream_T* const out,
  ZC_Err_T* const err
) {
  size_t lineNum = 1;
  for (;;++lineNum) {
    const ZC_Line_T line = ZC_tokenize(in, err);
    if (err->err != ZC_ERR_OK) return lineNum;
    if (line.tokens[0].eof) return lineNum;
    if (line.tokens[0].eol && line.tokens[0].str[0] == 0) continue;
    const ZA_Inst_T inst = ZC_parse(line, err);
    if (err->err != ZC_ERR_OK) return lineNum;
    const uint8_t code = ZC_generate(inst, err);
    if (err->err != ZC_ERR_OK) return lineNum;
    STM_Err_T serr = {0};
    STM_put(out, code, &serr);
    if (serr.err != STM_ERR_OK) {
      err->err = ZC_ERR_STM;
      err->code.stm = serr;
      return lineNum;
    }
  }
}

const char* ZC_getErrMsg(const ZC_Err_T err) {
  if (err.err == ZC_ERR_STM) return STM_getErrMsg(err.code.stm);
  if (err.err >= sizeof(s_ZC_ErrMsg) / sizeof(s_ZC_ErrMsg[0])) {
    return "unknown error";
  }
  return s_ZC_ErrMsg[err.err];
}

#if ZA_TGT == ZA_TGT_C
#include "zasmcli.h"

int main(int argc, char* const argv[]) {
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_IN, "src", {.stream = {0}}},
    {ZCLI_ARG_STREAM_OUT, "out", {.stream = {0}}},
  };
  ZCLI_ArgList_T arglist = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&arglist, argc, argv);
  ZC_Err_T err = {0};
  const size_t line_num = ZC_compile(&args[0].value.stream, &args[1].value.stream, &err);
  ZCLI_freeArgs(&arglist);
  if (err.err != ZC_ERR_OK) {
    ZCLI_error("at line %zu: %s", line_num, ZC_getErrMsg(err));
    return 1;
  }
  return 0;
}
#endif

