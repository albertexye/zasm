#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stream.h"

#define ZCLI_COLOR_ERROR   "\x1b[31m"
#define ZCLI_COLOR_SUCCESS "\x1b[32m"
#define ZCLI_COLOR_WARNING "\x1b[33m"
#define ZCLI_COLOR_IGNORE  "\x1b[90m"
#define ZCLI_COLOR_RESET   "\x1b[0m"

void ZCLI_usage(const char* usage);

void ZCLI_error(const char* msg, ...);

void ZCLI_errno(void);

void ZCLI_showmem(const uint8_t* mem, size_t len);

bool ZCLI_filetype(const char* name, char type);

STM_Stream_T ZCLI_openfile(const char* path, bool read);

void ZCLI_closefile(STM_Stream_T* stream);

void ZCLI_printbin(uint8_t byte);

bool ZCLI_readbin(uint8_t* byte);

void ZCLI_clearin(void);

bool ZCLI_getcmd(char* cmd);

bool ZCLI_inputfile(STM_Stream_T* stream, const char* prompt, bool read);

