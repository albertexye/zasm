/**
 * @file zasmm.h
 * @brief Microcode translation and mapping for ZASM (Z80-like Assembler/Microcode Project).
 *
 * This header provides types and functions for translating assembler instructions
 * into microcode, mapping microcode to hardware pin layouts, and generating
 * microcode output for use in hardware or simulation. It defines the core
 * data structures and API for the microcode generator and pin mapping logic.
 *
 * All functions and types are designed for use in the ZASM toolchain and are
 * intended for developers working on assembler, microcode, or hardware integration.
 */

#pragma once

#include <stdint.h>

#include "stream.h"
#include "zasm.h"

/**
 * @brief Output pin mapping for microcode signals.
 *
 * Array of hardware pin numbers corresponding to the 13 output signals.
 */
extern const uint8_t ZM_OUT_PINS[13];

/**
 * @brief Input pin mapping for microcode signals.
 *
 * Array of hardware pin numbers corresponding to the 8 input signals.
 */
extern const uint8_t ZM_IN_PINS[8];

/** @brief Pin number for the subtraction control signal. */
extern const uint8_t ZM_SB_PIN;
/** @brief Pin number for the CN (conditional negation) control signal. */
extern const uint8_t ZM_CN_PIN;
/** @brief Pin number for the HT (halt) control signal. */
extern const uint8_t ZM_HT_PIN;

/**
 * @brief Microcode signal structure.
 *
 * Represents the set of active microcode signals for a single instruction.
 * - out: Output signals (13 bits)
 * - in: Input signals (8 bits)
 * - sb: Subtraction control
 * - cn: Conditional negation
 * - rs: Reset control
 * - ht: Halt control
 */
typedef struct {
  bool out[13];
  bool in[8];
  bool sb, cn, ht;
} ZM_Code_T;

/**
 * @brief Pin layout structure for microcode output.
 *
 * Represents the hardware pin states for a single microcode cycle.
 * - pins: Array of 3 bytes, each bit representing a pin state.
 */
typedef struct {
  uint8_t pins[3];
} ZM_Layout_T;

/**
 * @brief Translate an assembler instruction to microcode signals.
 *
 * @param inst The assembler instruction to translate.
 * @return The corresponding microcode signals.
 */
ZM_Code_T ZM_translate(ZA_Inst_T inst);

/**
 * @brief Invert active-low pins in microcode signals.
 *
 * @param code The microcode signals (active-high).
 * @return The microcode signals with active-low logic applied.
 */
ZM_Code_T ZM_activeLow(ZM_Code_T code);

/**
 * @brief Map microcode signals to hardware pin layout.
 *
 * @param code The microcode signals.
 * @return The hardware pin layout for the given microcode.
 */
ZM_Layout_T ZM_map(ZM_Code_T code);

/**
 * @brief Generate the full pin layout for a given instruction code.
 *
 * @param code The 8-bit instruction code.
 * @return The hardware pin layout for the instruction.
 */
ZM_Layout_T ZM_macrocode(uint8_t code);

/**
 * @brief Generate and write the microcode output for a page.
 *
 * Writes the pin layout for all 256 instruction codes to the output stream for the specified page.
 *
 * @param out Output stream to write to.
 * @param page Page number (0, 1, or 2).
 * @param err Error structure for reporting errors.
 */
void ZM_generate(STM_Stream_T* out, uint8_t page, STM_Err_T* err);
