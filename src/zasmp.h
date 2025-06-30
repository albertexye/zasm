/**
 * @file zasmp.h
 * @brief Zero Page (ZP) packing and conversion utilities for ZASM.
 *
 * This header provides the interface for packing and converting 256-byte blocks
 * for different ZP target formats (instruction, number, microcode) as used in the assembler.
 *
 * The main function, ZP_pack, reads a 256-byte block from an input stream, transforms it
 * according to the specified target, and writes the result to an output stream.
 *
 * Usage is intended for assembler tools and utilities that need to manipulate ZP data.
 */
#pragma once

#include "stream.h"

/**
 * @enum ZP_Target_E
 * @brief Target format for Zero Page packing.
 *
 * Specifies the type of transformation to apply to the 256-byte block.
 */
typedef enum {
  /** Instruction table transformation. */
  ZP_TARGET_INST,
  /** Number table transformation. */
  ZP_TARGET_NUMBER,
  /** Microcode table transformation. */
  ZP_TARGET_MICROCODE,
} ZP_Target_E;

/**
 * @brief Packs and transforms a 256-byte block for a given ZP target.
 *
 * Reads 256 bytes from the input stream, applies the transformation specified by @p target,
 * and writes the result to the output stream. Sets @p err if an error occurs during I/O.
 *
 * @param[in]  in     Input stream to read 256 bytes from.
 * @param[out] out    Output stream to write 256 bytes to.
 * @param[in]  target Target format for transformation (see ZP_Target_E).
 * @param[out] err    Error structure to receive error information.
 */
void ZP_pack(STM_Stream_T* in, STM_Stream_T* out, ZP_Target_E target, STM_Err_T* err);

