#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "zasm.h"
#include "zasmd.h"
#include "zasms.h"

void ZS_new(ZS_State_T* const state) {
  state->halted = false;
  state->pc = 0;
  for (int i = 0; i < 7; ++i)
    state->r[i] = rand() & 0xFF;
  for (int i = 0; i < 256; ++i)
    state->mem[i] = rand() & 0xFF;
}

static uint8_t ZS_getReg(const ZS_State_T* const state, const ZA_Reg_E reg) {
  switch (reg) {
    case ZA_REG_A:
    case ZA_REG_C:
    case ZA_REG_G:
    case ZA_REG_X:
    case ZA_REG_Y:
      return state->r[reg];
    case ZA_REG_B:
      return state->b;
    case ZA_REG_D:
      return state->r[ZA_REG_X] - state->r[ZA_REG_Y];
    case ZA_REG_J:
      return state->r[ZA_REG_C] != 0;
    case ZA_REG_L:
      return (uint8_t)(state->r[ZA_REG_A] << 4);
    case ZA_REG_M:
      return state->mem[state->r[ZA_REG_A]];
    case ZA_REG_P:
      return state->pc;
    case ZA_REG_S:
      return state->r[ZA_REG_X] + state->r[ZA_REG_Y];
    case ZA_REG_N:
    case ZA_REG_Z:
      return 0;
    default:
      assert(false);
  }
}

static void ZS_resetState(ZS_State_T* const state) {
  memset(state->r, 0, sizeof(state->r));
}

void ZS_exec(ZS_State_T* const state) {
  if (state->halted) return;
  const uint8_t code = state->rom[state->pc];
  const ZA_Inst_T inst = ZD_parse(code);
  switch (inst.op) {
    case ZA_OP_MOV:
      state->r[inst.val.mov.r1] = ZS_getReg(state, inst.val.mov.r2);
      break;
    case ZA_OP_LDI:
      state->r[inst.val.ldi.r] = inst.val.ldi.i;
      break;
    case ZA_OP_JEZ:
      if (state->r[ZA_REG_C] == 0) {
        state->pc = ZS_getReg(state, inst.val.jmp);
        return;
      }
      break;
    case ZA_OP_JNZ:
      if (state->r[ZA_REG_C]) {
        state->pc = ZS_getReg(state, inst.val.jmp);
        return;
      }
      break;
    case ZA_OP_JNI:
      if (state->r[ZA_REG_C]) {
        state->pc = inst.val.imm;
        return;
      }
      break;
    case ZA_OP_HLT:
      state->halted = true;
      return;
    case ZA_OP_RST:
      ZS_resetState(state);
      break;
    default:
      assert(false);
  }
  ++state->pc;
}

#if ZA_TGT == ZA_TGT_S
#include <stdio.h>

#include "stream.h"
#include "zasmcli.h"

static const char* const USAGE = "zasms [program]";
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

static void printState(const ZS_State_T* const state) {
  if (state->halted) printf("halted\n");
  else printf("running\n");
  printReg('p', state->pc);
  printReg('b', state->b);
  for (int i = 0; i < 7; ++i) {
    printReg(g_ZA_RegNames[i], state->r[i]);
  }
}

static void run(ZS_State_T* const state) {
  for (size_t i = 0; i < 1024; ++i) {
    Zasms_exec(state);
    if (state->halted) {
      printf("halt\n");
      return;
    }
  }
  printf("reached 1024 instructions\n");
}

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
      Zasms_exec(state);
      break;
    default:
      ZCLI_error("unrecognized command - 'h' for help");
      break;
  }
}

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
  if (argc != 2) {
    ZCLI_error("bad arguments");
    ZCLI_usage(USAGE);
    return 1;
  }
  STM_Stream_T program = ZCLI_openfile(argv[1], true);
  ZS_State_T state;
  ZS_new(&state);
  STM_Err_T err;
  const size_t len = STM_read(&program, state.rom, sizeof(state.rom), &err);
  ZCLI_closefile(&program);
  if (err.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  printf("program loaded with %zu instructions\n", len);
  interactive(&state);
}
#endif

