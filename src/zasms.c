/**
 * @file zasms.c
 * @brief Implementation of the ZASM virtual machine state and execution logic.
 *
 * This file contains the implementation of the ZASM virtual machine core,
 * including state initialization, instruction execution, and interactive
 * debugging utilities (when built for the simulator target). Static helper
 * functions are declared and documented at the top of the file for clarity.
 *
 * Note: Exported functions are documented in the corresponding header file.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "zasm.h"
#include "zasmd.h"
#include "zasms.h"

/**
 * @brief Retrieves the value of a register or special register from the ZASM state.
 *
 * @param state Pointer to the ZS_State_T structure.
 * @param reg Register identifier (ZA_Reg_E).
 * @return Value of the specified register.
 *
 * Handles both general-purpose and special registers.
 */
static uint8_t ZS_getReg(const ZS_State_T* state, ZA_Reg_E reg);

/**
 * @brief Resets all general-purpose registers in the ZASM state to zero.
 *
 * @param state Pointer to the ZS_State_T structure.
 */
static void ZS_resetState(ZS_State_T* state);

static uint8_t ZS_getReg(const ZS_State_T* const state, const ZA_Reg_E reg) {
  switch (reg) {
    case ZA_REG_A:
    case ZA_REG_C:
    case ZA_REG_G:
    case ZA_REG_X:
    case ZA_REG_Y:
      // Return value from general-purpose register
      return state->r[reg];
    case ZA_REG_B:
      return state->b;
    case ZA_REG_D:
      // D is defined as X - Y
      return state->r[ZA_REG_X] - state->r[ZA_REG_Y];
    case ZA_REG_J:
      // J is 1 if C != 0, else 0
      return state->r[ZA_REG_C] != 0;
    case ZA_REG_L:
      // L is A shifted left by 4
      return (uint8_t)(state->r[ZA_REG_A] << 4);
    case ZA_REG_M:
      // M is memory at address in A
      return state->mem[state->r[ZA_REG_A]];
    case ZA_REG_P:
      return state->pc;
    case ZA_REG_S:
      // S is X + Y
      return state->r[ZA_REG_X] + state->r[ZA_REG_Y];
    case ZA_REG_N:
    case ZA_REG_Z:
      // N and Z are always 0
      return 0;
    default:
      assert(false);
  }
}

static void ZS_resetState(ZS_State_T* const state) {
  // Zero all general-purpose registers
  memset(state->r, 0, sizeof(state->r));
}

void ZS_new(ZS_State_T* const state) {
  state->halted = false;
  state->pc = 0;
  // Initialize general-purpose registers with random values
  for (int i = 0; i < 7; ++i)
    state->r[i] = rand() & 0xFF;
  // Initialize memory with random values
  for (int i = 0; i < 256; ++i)
    state->mem[i] = rand() & 0xFF;
}

void ZS_exec(ZS_State_T* const state) {
  if (state->halted) return;
  // Fetch instruction code from ROM at current PC
  const uint8_t code = state->rom[state->pc];
  // Decode instruction
  const ZA_Inst_T inst = ZD_parse(code);
  switch (inst.op) {
    case ZA_OP_MOV:
      // Move value from one register to another
      if (inst.val.mov.r1 == ZA_REG_M) state->mem[state->r[ZA_REG_A]] = ZS_getReg(state, inst.val.mov.r2);
      else state->r[inst.val.mov.r1] = ZS_getReg(state, inst.val.mov.r2);
      break;
    case ZA_OP_LDI:
      // Load immediate value into register
      state->r[inst.val.ldi.r] = inst.val.ldi.i;
      break;
    case ZA_OP_JEZ:
      // Jump if C == 0
      if (state->r[ZA_REG_C] == 0) {
        state->pc = ZS_getReg(state, inst.val.jmp);
        return;
      }
      break;
    case ZA_OP_JNZ:
      // Jump if C != 0
      if (state->r[ZA_REG_C]) {
        state->pc = ZS_getReg(state, inst.val.jmp);
        return;
      }
      break;
    case ZA_OP_JNI:
      // Jump to immediate if C != 0
      if (state->r[ZA_REG_C]) {
        state->pc = inst.val.imm;
        return;
      }
      break;
    case ZA_OP_HLT:
      // Halt execution
      state->halted = true;
      return;
    case ZA_OP_RST:
      // Reset registers
      ZS_resetState(state);
      break;
    default:
      assert(false);
  }
  // Advance program counter
  ++state->pc;
}

#if ZA_TGT == ZA_TGT_S
#include <stdio.h>

#include "stream.h"
#include "zasmcli.h"

static const char* const COMMANDS =
  "commands:\n"
  "\tc - continue for at most 1024 instructions\n"
  "\th - help text\n"
  "\tm - print memory\n"
  "\tp - print state\n"
  "\tq - quit\n"
  "\tr - reset state\n"
  "\ts - step\n";

static void printReg(const char r, const uint8_t b) {
  printf("%c: u[%3hhu] i[%4hhd] h[%02hhx] b[", r, b, (int8_t)b, b);
  ZCLI_printbin(b);
  printf("]\n");
}

/**
 * @brief Prints the current state of the ZASM virtual machine.
 *
 * @param state Pointer to the ZS_State_T structure.
 */
static void printState(const ZS_State_T* const state) {
  if (state->halted) printf("halted\n");
  else printf("running\n");
  printReg('p', state->pc);
  printReg('b', state->b);
  for (int i = 0; i < 7; ++i) {
    printReg(g_ZA_RegNames[i], state->r[i]);
  }
}

/**
 * @brief Runs the ZASM state for up to 1024 instructions or until halted.
 *
 * @param state Pointer to the ZS_State_T structure.
 */
static void run(ZS_State_T* const state) {
  for (size_t i = 0; i < 1024; ++i) {
    ZS_exec(state);
    if (state->halted) {
      printf("halt\n");
      return;
    }
  }
  printf("reached 1024 instructions\n");
}

/**
 * @brief Executes a single interactive command on the ZASM state.
 *
 * @param state Pointer to the ZS_State_T structure.
 * @param cmd Command character.
 */
static void exec(ZS_State_T* const state, const char cmd) {
  STM_Err_T err;
  STM_Stream_T sstdout = STM_stdout();
  switch (cmd) {
    case 'b':
      printf("value>");
      if (!ZCLI_readbin(&state->b))
        ZCLI_error("invalid 8-bit binary number");
      break;
    case 'c':
      run(state);
      break;
    case 'h':
      printf(COMMANDS);
      break;
    case 'm':
      ZCLI_showmem(state->mem, sizeof(state->mem));
      break;
    case 'p':
      printState(state);
      break;
    case 'q':
      printf("exit\n");
      exit(0);
    case 'r':
      ZS_new(state);
      printf("reset state\n");
      break;
    case 's':
      ZA_explainInst(ZD_parse(state->rom[state->pc]), &sstdout, &err);
      printf("\n");
      ZS_exec(state);
      break;
    default:
      ZCLI_error("unrecognized command - 'h' for help");
      break;
  }
}

/**
 * @brief Starts the interactive command loop for the ZASM simulator.
 *
 * @param state Pointer to the ZS_State_T structure.
 * @note This function does not return.
 */
[[noreturn]] static void interactive(ZS_State_T* const state) {
  printf("zasms interactive\n");
  while (true) {
    char cmd;
    printf(">");
    if (!ZCLI_getcmd(&cmd)) {
      ZCLI_error("invalid command - 'h' for help");
      continue;
    }
    exec(state, cmd);
  }
}

int main(const int argc, char* const argv[]) {
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_IN, "program", {.stream = {0}}},
  };
  ZCLI_ArgList_T arglist = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&arglist, argc, argv);
  ZS_State_T state;
  ZS_new(&state);
  STM_Err_T err;
  const size_t len = STM_read(&args[0].value.stream, state.rom, sizeof(state.rom), &err);
  ZCLI_freeArgs(&arglist);
  if (err.err != STM_ERR_OK && err.err != STM_ERR_EOF) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  printf("program loaded with %zu instructions\n", len);
  interactive(&state);
}
#endif

