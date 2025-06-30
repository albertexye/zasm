/**
 * @file zasmd.h
 * @brief Disassembler interface for ZASM instructions.
 *
 * This header provides the interface for parsing and disassembling ZASM instructions.
 * It defines functions to convert raw instruction codes into structured instruction representations
 * and to disassemble instruction streams for output. Intended for use in ZASM disassembler tools
 * and utilities.
 */

#pragma once

#include <stdint.h>

#include "stream.h"
#include "zasm.h"

/**
 * @brief Parse a single ZASM instruction code into a structured instruction.
 *
 * This function decodes a raw 8-bit instruction code and returns a corresponding
 * ZA_Inst_T structure representing the parsed instruction and its operands.
 *
 * @param code The 8-bit instruction code to parse.
 * @return ZA_Inst_T The parsed instruction structure.
 */
ZA_Inst_T ZD_parse(uint8_t code);

/**
 * @brief Disassemble a stream of ZASM instructions.
 *
 * Reads instruction codes from the input stream, decodes them, and writes the
 * disassembled instructions to the output stream. Handles errors via the provided
 * error structure.
 *
 * @param in Pointer to the input stream containing instruction codes.
 * @param out Pointer to the output stream for disassembled instructions.
 * @param err Pointer to the error structure for reporting stream or decoding errors.
 */
void ZD_disassemble(STM_Stream_T* in, STM_Stream_T* out, STM_Err_T* err);

