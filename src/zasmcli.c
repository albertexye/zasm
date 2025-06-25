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

void ZCLI_usage(const char *const usage) {
  puts(ZCLI_COLOR_SUCCESS "usage" ZCLI_COLOR_RESET ": ");
  puts(usage);
  putchar('\n');
}

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
    if (i > 0 && i % 16 == 0)
      printf("\n%zux", i);
    if (i % 4 == 0)
      printf(" ");
    printf("%hhx", mem[i]);
  }
  printf("\n");
}

bool ZCLI_filetype(const char *const name, const char type) {
  const size_t len = strlen(name);
  return name[len - 1] == type && name[len - 2] == '.';
}

STM_Stream_T ZCLI_openfile(const char *const path, const bool read) {
  FILE *const file = fopen(path, read ? "rb" : "wb");
  if (file == NULL) {
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

bool ZCLI_inputfile(STM_Stream_T *const stream, const char *const prompt,
                       const bool read) {
  printf("%s", prompt);
  char path[256];
  if (fgets(path, sizeof(path), stdin) == NULL) {
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
