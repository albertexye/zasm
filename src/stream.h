#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum : uint8_t {
  STM_ERR_OK,
  STM_ERR_EOF,
  STM_ERR_ERRNO,
} STM_Err_E;

typedef struct {
  STM_Err_E err;
  int errno_;
} STM_Err_T;

typedef union {
  FILE* file;
  struct {
    union {
      uint8_t* w;
      const uint8_t* r;
    } ptr;
    size_t pos;
    size_t len;
  } buf;
} STM_StreamData_T;

typedef struct {
  STM_StreamData_T data;
  bool file;
  bool read;
  bool managed;
} STM_Stream_T;

STM_Stream_T STM_fromFile(FILE* file, bool read, bool managed);

STM_Stream_T STM_fromBuf(uint8_t* buf, size_t len, bool managed);

STM_Stream_T STM_fromConstBuf(const uint8_t* buf, size_t len, bool managed);

STM_Stream_T STM_stdin(void);

STM_Stream_T STM_stdout(void);

void STM_close(STM_Stream_T* stream, STM_Err_T* err);

uint8_t STM_get(STM_Stream_T* stream, STM_Err_T* err);

void STM_put(STM_Stream_T* stream, uint8_t byte, STM_Err_T* err);

size_t STM_read(STM_Stream_T* stream, void* buf, size_t len, STM_Err_T* err);

size_t STM_write(STM_Stream_T* stream, const void* buf, size_t len, STM_Err_T* err);

size_t STM_printf(STM_Stream_T* stream, STM_Err_T* err, const char* str, ...);

const char* STM_getErrMsg(STM_Err_T err);

