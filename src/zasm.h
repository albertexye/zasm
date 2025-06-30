/**
 * @file zasm.h
 * @brief ZASM instruction set definitions, types, and utility functions.
 *
 * This header provides the core types, macros, and function declarations for the ZASM assembler and disassembler.
 * It defines the instruction and register enums, argument structures, and utility functions for working with ZASM instructions.
 *
 * All functions and types are designed for use in ZASM tools and are suitable for both assembler and disassembler implementations.
 */

#pragma once

#include <stdint.h>

#include "stream.h"

/** @def ZA_TGT_C
 *  @brief Target code for C backend.
 */
#define ZA_TGT_C 1
/** @def ZA_TGT_D
 *  @brief Target code for D backend.
 */
#define ZA_TGT_D 2
/** @def ZA_TGT_S
 *  @brief Target code for S backend.
 */
#define ZA_TGT_S 3
/** @def ZA_TGT_M
 *  @brief Target code for M backend.
 */
#define ZA_TGT_M 4
/** @def ZA_TGT_F
 *  @brief Target code for F backend.
 */
#define ZA_TGT_F 5
/** @def ZA_TGT_P
 *  @brief Target code for P backend.
 */
#define ZA_TGT_P 6
/** @def ZA_TGT_N
 *  @brief Target code for N backend.
 */
#define ZA_TGT_N 7

#ifndef ZA_TGT
/** @def ZA_TGT
 *  @brief Default target code (0 = unspecified).
 */
#define ZA_TGT 0
#endif

/**
 * @enum ZA_Op_E
 * @brief ZASM operation codes.
 */
typedef enum {
  ZA_OP_MOV, /**< Move register to register. */
  ZA_OP_LDI, /**< Load immediate value into register. */
  ZA_OP_JEZ, /**< Jump if equal to zero. */
  ZA_OP_JNZ, /**< Jump if not zero. */
  ZA_OP_JNI, /**< Jump to immediate address. */
  ZA_OP_HLT, /**< Halt execution. */
  ZA_OP_RST, /**< Reset. */
} ZA_Op_E;

/**
 * @var g_ZA_OpNames
 * @brief String names for each operation code (indexed by ZA_Op_E).
 */
extern const char g_ZA_OpNames[7][4];

/**
 * @enum ZA_Reg_E
 * @brief ZASM register codes.
 */
typedef enum {
  ZA_REG_A, /**< Address. */
  ZA_REG_C, /**< Conditional. */
  ZA_REG_G, /**< General Purpose. */
  ZA_REG_M, /**< Memory. */
  ZA_REG_X, /**< Operand 1. */
  ZA_REG_Y, /**< Operand 2. */
  ZA_REG_N, /**< Number. */
  ZA_REG_P, /**< Program Counter. */
  ZA_REG_B, /**< Button Input. */
  ZA_REG_J, /**< Jump Condition. */
  ZA_REG_L, /**< Left Shift. */
  ZA_REG_S, /**< Sum. */
  ZA_REG_D, /**< Difference. */
  ZA_REG_Z, /**< Zero. */
} ZA_Reg_E;

/**
 * @var g_ZA_RegNames
 * @brief String names for each register (indexed by ZA_Reg_E).
 */
extern const char g_ZA_RegNames[14];

/**
 * @struct ZA_MovArgs_T
 * @brief Arguments for the MOV instruction.
 */
typedef struct {
  ZA_Reg_E r1; /**< Destination register. */
  ZA_Reg_E r2; /**< Source register. */
} ZA_MovArgs_T;

/**
 * @struct ZA_LdiArgs_T
 * @brief Arguments for the LDI instruction.
 */
typedef struct {
  ZA_Reg_E r; /**< Target register. */
  uint8_t i : 4; /**< Immediate value (4 bits). */
} ZA_LdiArgs_T;

/**
 * @typedef ZA_Args_T
 * @brief Union of all possible instruction arguments.
 */
typedef union {
  ZA_MovArgs_T mov; /**< MOV arguments. */
  ZA_LdiArgs_T ldi; /**< LDI arguments. */
  ZA_Reg_E jmp;     /**< Register for jump instructions. */
  uint8_t imm : 4;  /**< Immediate value for JNI. */
} ZA_Args_T;

/**
 * @struct ZA_Inst_T
 * @brief ZASM instruction structure.
 */
typedef struct {
  ZA_Op_E op;    /**< Operation code. */
  ZA_Args_T val; /**< Instruction arguments. */
} ZA_Inst_T;

/**
 * @brief Check if a ZASM instruction is valid.
 * @param inst The instruction to check.
 * @return true if the instruction is valid, false otherwise.
 */
bool ZA_checkInst(ZA_Inst_T inst);

/**
 * @brief Print a human-readable explanation of a ZASM instruction.
 * @param inst The instruction to explain.
 * @param out Output stream.
 * @param eof Error output.
 */
void ZA_explainInst(ZA_Inst_T inst, STM_Stream_T* out, STM_Err_T* eof);

/**
 * @brief Print a ZASM instruction in assembly format.
 * @param inst The instruction to print.
 * @param out Output stream.
 * @param eof Error output.
 */
void ZA_printInst(ZA_Inst_T inst, STM_Stream_T* out, STM_Err_T* eof);

