/**
 * @file zasmc.h
 * @brief ZASM Compiler core interface. Provides tokenization, parsing, code generation, and error handling for the ZASM assembler.
 *
 * This header defines the main types and functions for compiling ZASM assembly code into machine code.
 * It includes error handling, token and line representations, and the main compilation pipeline.
 *
 * All functions and types are designed for use by the assembler core and CLI frontends.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stream.h"
#include "zasm.h"

/**
 * @enum ZC_Err_E
 * @brief Error codes for the ZASM compiler.
 */
typedef enum {
  ZC_ERR_OK,           /**< No error. */
  ZC_ERR_OP,           /**< Invalid operation. */
  ZC_ERR_REG,          /**< Invalid register. */
  ZC_ERR_IMM_BASE,     /**< Invalid immediate base. */
  ZC_ERR_IMM_DIGIT,    /**< Invalid immediate digit. */
  ZC_ERR_IMM_LEN,      /**< Bad immediate length. */
  ZC_ERR_IMM_OVERFLOW, /**< Immediate overflow. */
  ZC_ERR_INVAL_CHAR,   /**< Invalid character. */
  ZC_ERR_TOKEN_LEN,    /**< Bad token length. */
  ZC_ERR_LINE_LEN,     /**< Bad line length. */
  ZC_ERR_INST_FMT,     /**< Bad instruction format. */
  ZC_ERR_RO_REG,       /**< Writing to read-only register. */
  ZC_ERR_STM,          /**< Stream error. */
} ZC_Err_E;

/**
 * @struct ZC_Err_T
 * @brief Error structure for ZASM compiler operations.
 *
 * Contains the error code and additional information for stream errors.
 */
typedef struct {
  ZC_Err_E err; /**< Error code. */
  union {
    STM_Err_T stm; /**< Stream error details. */
  } code;
} ZC_Err_T;

/**
 * @struct ZC_Token_T
 * @brief Represents a token parsed from the input stream.
 *
 * Contains the token string and end-of-line/end-of-file flags.
 */
typedef struct {
  char str[7]; /**< Token string (null-terminated, max 6 chars). */
  bool eol: 1; /**< End of line flag. */
  bool eof: 1; /**< End of file flag. */
} ZC_Token_T;

/**
 * @struct ZC_Line_T
 * @brief Represents a line of tokens parsed from the input stream.
 */
typedef struct {
  ZC_Token_T tokens[3]; /**< Array of tokens in the line. */
} ZC_Line_T;

/**
 * @brief Tokenizes a line from the input stream.
 *
 * @param stream Input stream to read from.
 * @param err Output error structure.
 * @return Parsed line of tokens.
 */
ZC_Line_T ZC_tokenize(STM_Stream_T* stream, ZC_Err_T* err);

/**
 * @brief Parses a line of tokens into an instruction.
 *
 * @param line Line of tokens to parse.
 * @param err Output error structure.
 * @return Parsed instruction.
 */
ZA_Inst_T ZC_parse(ZC_Line_T line, ZC_Err_T* err);

/**
 * @brief Generates machine code for a parsed instruction.
 *
 * @param inst Parsed instruction.
 * @param err Output error structure.
 * @return Generated machine code byte.
 */
uint8_t ZC_generate(ZA_Inst_T inst, ZC_Err_T* err);

/**
 * @brief Compiles the input stream to the output stream.
 *
 * Reads lines, tokenizes, parses, generates code, and writes to output.
 *
 * @param in Input stream.
 * @param out Output stream.
 * @param err Output error structure.
 * @return Line number where error occurred or total lines processed.
 */
size_t ZC_compile(STM_Stream_T* in, STM_Stream_T* out, ZC_Err_T* err);

/**
 * @brief Returns a human-readable error message for a given error.
 *
 * @param err Error structure.
 * @return Error message string.
 */
const char* ZC_getErrMsg(ZC_Err_T err);
