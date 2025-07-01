/**
 * @file zasmcli.c
 * @brief Implementation of command-line interface utilities for the ZASM assembler project.
 *
 * This file provides the implementation of CLI helpers for ZASM, including argument parsing,
 * file I/O, error reporting, and user interaction. Static helper functions are declared and documented
 * at the top of the file. Exported functions are documented in the corresponding header.
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "stream.h"
#include "zasmcli.h"

/**
 * @brief Print the usage format for a list of command-line arguments.
 *
 * This static helper prints a usage string to stdout, showing the expected arguments
 * and their types for a given argument list.
 *
 * @param args Pointer to the argument list structure.
 */
static void ZCLI_printArgsFmt(const ZCLI_ArgList_T *args);

void ZCLI_error(const char *const msg, ...) {
  fputs(ZCLI_COLOR_ERROR "error" ZCLI_COLOR_RESET ": ", stderr);
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fputc('\n', stderr);
}

void ZCLI_errno(void) {
  fprintf(stderr, ZCLI_COLOR_ERROR "error" ZCLI_COLOR_RESET ": %s\n",
          strerror(errno));
}

void ZCLI_showmem(const uint8_t *const mem, const size_t len) {
  for (size_t i = 0; i < len; ++i) {
    // Print a new line every 16 bytes, with offset
    if (i % 16 == 0)
      printf("\n%02x|", (uint8_t)i);
    // Add a space every 4 bytes for readability
    if (i % 4 == 0)
      printf(" ");
    printf("%02x", mem[i]);
  }
  printf("\n");
}

STM_Stream_T ZCLI_openfile(const char *const path, const bool read) {
  FILE *const file = fopen(path, read ? "rb" : "wb");
  if (file == nullptr) {
    ZCLI_errno();
    exit(1);
  }
  return STM_fromFile(file, read, true);
}

void ZCLI_closefile(STM_Stream_T *const stream) {
  STM_Err_T serr = {0};
  STM_close(stream, &serr);
  if (serr.err != STM_ERR_OK)
    ZCLI_error("failed to close file: %s", STM_getErrMsg(serr));
}

void ZCLI_printbin(const uint8_t byte) {
  for (int i = 0; i < 8; ++i) {
    putchar('0' + ((byte >> (7 - i)) & 1));
  }
}

bool ZCLI_readbin(uint8_t *const byte) {
  uint8_t b = 0;
  for (int i = 0; i < 8; ++i) {
    b <<= 1;
    const int c = getchar();
    if (c == EOF || c == '\n')
      return false;
    if (c != '0' && c != '1')
      return false;
    b += c - '0';
  }
  if (getchar() == '\n') {
    *byte = b;
    return true;
  }
  ZCLI_clearin();
  return false;
}

void ZCLI_clearin(void) {
  while (true) {
    const int c = getchar();
    if (c == EOF || c == '\n')
      return;
  }
}

bool ZCLI_getcmd(char *const cmd) {
  int c = getchar();
  if (c == EOF || c == '\n')
    return false;
  *cmd = (char)c;
  if (getchar() == '\n')
    return true;
  ZCLI_clearin();
  return false;
}

bool ZCLI_inputfile(STM_Stream_T *const stream, const char *const prompt, const bool read) {
  printf("%s", prompt);
  char path[256];
  if (fgets(path, sizeof(path), stdin) == nullptr) {
    ZCLI_errno();
    return false;
  }
  if (path[strlen(path) - 1] != '\n') {
    ZCLI_clearin();
    ZCLI_error("path is too long");
    return false;
  }
  path[strlen(path) - 1] = '\0'; // Remove newline
  struct stat st;
  if (stat(path, &st) == -1) {
    ZCLI_errno();
    return false;
  }
  *stream = ZCLI_openfile(path, read);
  return true;
}

static void ZCLI_printArgsFmt(const ZCLI_ArgList_T *const args) {
  printf(ZCLI_COLOR_SUCCESS "usage:" ZCLI_COLOR_RESET);
  for (size_t i = 0; i < args->len; ++i) {
    printf(" %s:", args->args[i].name);
    switch (args->args[i].type) {
      case ZCLI_ARG_STREAM_IN:
        printf("<in>");
        break;
      case ZCLI_ARG_STREAM_OUT:
        printf("<out>");
        break;
      case ZCLI_ARG_CHAR:
        printf("<char>");
        break;
      default:
        assert(false);
    }
  }
  printf("\n");
}

void ZCLI_parseArgs(
  ZCLI_ArgList_T *const args,
  int argc, char *const argv[]
) {
  if ((size_t)argc - 1 != args->len) {
    ZCLI_error("bad number of arguments");
    ZCLI_printArgsFmt(args);
    goto error;
  }
  for (size_t i = 0; i < args->len; ++i) {
    switch (args->args[i].type) {
      case ZCLI_ARG_STREAM_IN:
        args->args[i].value.stream = ZCLI_openfile(argv[i + 1], true);
        break;
      case ZCLI_ARG_STREAM_OUT:
        args->args[i].value.stream = ZCLI_openfile(argv[i + 1], false);
        break;
      case ZCLI_ARG_CHAR:
        if (strlen(argv[i + 1]) != 1) {
          ZCLI_error("argument '%s' must be a single character", args->args[i].name);
          goto error;
        }
        args->args[i].value.c = argv[i + 1][0];
        break;
      default:
        assert(false);
    }
  }
  return;
error:
  ZCLI_freeArgs(args);
  exit(1);
}

void ZCLI_freeArgs(ZCLI_ArgList_T *const args) {
  for (size_t i = 0; i < args->len; ++i) {
    if (args->args[i].type == ZCLI_ARG_STREAM_IN ||
        args->args[i].type == ZCLI_ARG_STREAM_OUT) {
      STM_Err_T err = {0};
      STM_close(&args->args[i].value.stream, &err);
      if (err.err != STM_ERR_OK)
        ZCLI_error("failed to close stream: %s", STM_getErrMsg(err));
    }
  }
}

