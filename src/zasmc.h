#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stream.h"
#include "zasm.h"

typedef enum {
  ZC_ERR_OK,
  ZC_ERR_OP,
  ZC_ERR_REG,
  ZC_ERR_IMM_BASE,
  ZC_ERR_IMM_DIGIT,
  ZC_ERR_IMM_LEN,
  ZC_ERR_IMM_OVERFLOW,
  ZC_ERR_INVAL_CHAR,
  ZC_ERR_TOKEN_LEN,
  ZC_ERR_LINE_LEN,
  ZC_ERR_INST_FMT,
  ZC_ERR_RO_REG,
  ZC_ERR_STM,
} ZC_Err_E;

typedef struct {
  ZC_Err_E err;
  union {
    STM_Err_T stm;
  } code;
} ZC_Err_T;

typedef struct {
  char str[7];
  bool eol: 1;
  bool eof: 1;
} ZC_Token_T;

typedef struct {
  ZC_Token_T tokens[3];
} ZC_Line_T;

ZC_Line_T ZC_tokenize(STM_Stream_T* stream, ZC_Err_T* err);

ZA_Inst_T ZC_parse(ZC_Line_T line, ZC_Err_T* err);

uint8_t ZC_generate(ZA_Inst_T inst, ZC_Err_T* err);

size_t ZC_compile(STM_Stream_T* in, STM_Stream_T* out, ZC_Err_T* err);

const char* ZC_getErrMsg(ZC_Err_T err);
