#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stream.h"

STM_Stream_T STM_fromFile(
  FILE* const file,
  const bool read,
  const bool managed
) {
  return (STM_Stream_T) {
    .file = true,
    .read = read,
    .managed = managed,
    .data.file = file,
  };
}

STM_Stream_T STM_fromBuf(
  uint8_t* const buf,
  const size_t len,
  const bool managed
) {
  return (STM_Stream_T) {
    .file = false,
    .read = false,
    .managed = managed,
    .data.buf = {
      .ptr.w = buf,
      .pos = 0,
      .len = len,
    },
  };
}

STM_Stream_T STM_fromConstBuf(
  const uint8_t* const buf,
  const size_t len,
  const bool managed
) {
  return (STM_Stream_T) {
    .file = false,
    .read = true,
    .managed = managed,
    .data.buf = {
      .ptr.r = buf,
      .pos = 0,
      .len = len,
    },
  };
}

STM_Stream_T STM_stdin(void) {
  return STM_fromFile(stdin, true, false);
}

STM_Stream_T STM_stdout(void) {
  return STM_fromFile(stdout, false, false);
}

static void STM_checkErrno(STM_Err_T* const err) {
  if (errno == 0) {
    err->err = STM_ERR_EOF;
  } else {
    err->err = STM_ERR_ERRNO;
    err->errno_ = errno;
  }
}

void STM_close(STM_Stream_T* const stream, STM_Err_T* const err) {
  if (!stream->managed) goto end;
  if (stream->file) {
    if (fclose(stream->data.file) == EOF) {
      err->err = STM_ERR_ERRNO;
      err->errno_ = errno;
      return;
    }
  } else {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-qual"
    if (stream->read) free((void*)stream->data.buf.ptr.r);
    #pragma GCC diagnostic pop
    else free(stream->data.buf.ptr.w);
  }
end:
  *stream = (STM_Stream_T) { 0 };
}

uint8_t STM_get(STM_Stream_T* const stream, STM_Err_T* const err) {
  assert(stream->read);
  if (stream->file) {
    const int c = fgetc(stream->data.file);
    if (c == EOF) {
      STM_checkErrno(err);
      return 0;
    }
    return (uint8_t)(c & 0xFF);
  } else {
    if (stream->data.buf.pos == stream->data.buf.len) {
      err->err = STM_ERR_EOF;
      return 0;
    }
    return stream->data.buf.ptr.r[stream->data.buf.pos++];
  }
}

void STM_put(STM_Stream_T* const stream, const uint8_t byte, STM_Err_T* const err) {
  assert(!stream->read);
  if (stream->file) {
    if (fputc(byte, stream->data.file) == EOF)
      STM_checkErrno(err);
  } else {
    if (stream->data.buf.pos == stream->data.buf.len) {
      err->err = STM_ERR_EOF;
      return;
    }
    stream->data.buf.ptr.w[stream->data.buf.pos++] = byte;
  }
}

size_t STM_read(STM_Stream_T* const stream, void* const buf, const size_t len, STM_Err_T* const err) {
  uint8_t* const mem = buf;
  for (size_t i = 0; i < len; ++i) {
    mem[i] = STM_get(stream, err);
    if (err->err != STM_ERR_OK) return i;
  }
  return len;
}

size_t STM_write(STM_Stream_T* const stream, const void* const buf, const size_t len, STM_Err_T* const err) {
  const uint8_t* const mem = buf;
  for (size_t i = 0; i < len; ++i) {
    STM_put(stream, mem[i], err);
    if (err->err != STM_ERR_OK) return i;
  }
  return len;
}

size_t STM_printf(
  STM_Stream_T* const stream,
  STM_Err_T* const err,
  const char* const str,
  ...
) {
  assert(!stream->read);
  va_list arg;
  size_t len = 0;
  va_start(arg, str);
  if (stream->file) {
    const int ret = vfprintf(stream->data.file, str, arg);
    if (ret < 0) STM_checkErrno(err);
    else len = (size_t)ret;
  } else {
    const size_t avail = stream->data.buf.len - stream->data.buf.pos;
    const int ret = vsnprintf(
      (char*)stream->data.buf.ptr.w + stream->data.buf.pos,
      avail,
      str,
      arg
    );
    if (ret < 0) {
      STM_checkErrno(err);
    } else if ((size_t)ret >= avail) {
      err->err = STM_ERR_EOF;
      len = avail;
    } else {
      len = (size_t)ret + 1;
    }
    stream->data.buf.pos += len;
  }
  va_end(arg);
  return len;
}

const char* STM_getErrMsg(const STM_Err_T err) {
  switch (err.err) {
    case STM_ERR_OK:
      return "no error";
    case STM_ERR_EOF:
      return "end of file";
    case STM_ERR_ERRNO:
      return strerror(err.errno_);
    default:
      return "unknown error";
  }
}

