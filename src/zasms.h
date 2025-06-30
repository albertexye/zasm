/**
 * @file zasms.h
 * @brief State and execution functions for the ZASM virtual machine.
 *
 * This header defines the state structure and core functions for the ZASM
 * virtual machine, including memory, registers, and execution control.
 * It provides the interface for initializing and executing instructions
 * on the ZASM state. Suitable for use in emulators, debuggers, or other
 * tools that interact with the ZASM architecture.
 */

#pragma once

#include <stdint.h>

/**
 * @struct ZS_State_T
 * @brief Represents the complete state of the ZASM virtual machine.
 *
 * Contains memory, ROM, general-purpose registers, a special register,
 * program counter, and a halted flag.
 */
typedef struct {
  uint8_t mem[256];   /**< Main memory (256 bytes). */
  uint8_t rom[256];   /**< Program ROM (256 bytes). */
  uint8_t r[7];       /**< General-purpose registers (A, C, G, X, Y, etc.). */
  uint8_t b;          /**< Special register B. */
  uint8_t pc;         /**< Program counter. */
  bool halted;        /**< Halted state flag. */
} ZS_State_T;

/**
 * @brief Initializes a ZS_State_T structure with random values.
 *
 * Sets the program counter to zero, clears the halted flag, and fills
 * registers and memory with random values.
 * @param state Pointer to the ZS_State_T structure to initialize.
 */
void ZS_new(ZS_State_T* state);

/**
 * @brief Executes a single instruction on the ZASM state.
 *
 * Decodes and executes the instruction at the current program counter.
 * Updates the state accordingly, including registers, memory, and halted flag.
 * @param state Pointer to the ZS_State_T structure to execute on.
 */
void ZS_exec(ZS_State_T* state);

