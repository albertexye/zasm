/**
 * @file stream.h
 * @brief Stream abstraction for file and memory buffer I/O operations.
 *
 * This header provides a unified interface for reading from and writing to files or memory buffers.
 * It defines types and functions for stream management, error handling, and formatted I/O.
 *
 * Usage:
 *   - Create a stream from a file or buffer using the provided constructors.
 *   - Use STM_get, STM_put, STM_read, STM_write, and STM_printf for I/O operations.
 *   - Handle errors using STM_Err_T and STM_getErrMsg.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @enum STM_Err_E
 * @brief Error codes for stream operations.
 */
typedef enum : uint8_t {
  STM_ERR_OK,      /**< No error occurred. */
  STM_ERR_EOF,     /**< End of file or buffer reached. */
  STM_ERR_ERRNO,   /**< Error indicated by errno. */
} STM_Err_E;

/**
 * @struct STM_Err_T
 * @brief Structure representing a stream error.
 * @var STM_Err_T::err Error code (STM_Err_E).
 * @var STM_Err_T::errno_ System errno value if err == STM_ERR_ERRNO.
 */
typedef struct {
  STM_Err_E err;
  int errno_;
} STM_Err_T;

/**
 * @typedef STM_StreamData_T
 * @brief Union holding either a file pointer or buffer information for a stream.
 * @var STM_StreamData_T::file File pointer for file-based streams.
 * @var STM_StreamData_T::buf Buffer information for memory-based streams.
 */
typedef union {
  FILE* file; /**< File pointer. */
  struct {
    union {
      uint8_t* w;        /**< Writable buffer pointer. */
      const uint8_t* r;  /**< Read-only buffer pointer. */
    } ptr;
    size_t pos;          /**< Current position in buffer. */
    size_t len;          /**< Total length of buffer. */
  } buf;
} STM_StreamData_T;

/**
 * @struct STM_Stream_T
 * @brief Stream object for file or buffer I/O.
 * @var STM_Stream_T::data Stream data (file or buffer).
 * @var STM_Stream_T::file True if stream is file-based.
 * @var STM_Stream_T::read True if stream is readable.
 * @var STM_Stream_T::managed True if stream manages the lifetime of its resource.
 */
typedef struct {
  STM_StreamData_T data;
  bool file;
  bool read;
  bool managed;
} STM_Stream_T;

/**
 * @brief Create a stream from a FILE pointer.
 * @param file FILE pointer.
 * @param read True for reading, false for writing.
 * @param managed If true, stream will close file on STM_close.
 * @return Initialized STM_Stream_T.
 */
STM_Stream_T STM_fromFile(FILE* file, bool read, bool managed);

/**
 * @brief Create a stream from a writable memory buffer.
 * @param buf Pointer to buffer.
 * @param len Length of buffer.
 * @param managed If true, stream will free buffer on STM_close.
 * @return Initialized STM_Stream_T.
 */
STM_Stream_T STM_fromBuf(uint8_t* buf, size_t len, bool managed);

/**
 * @brief Create a stream from a read-only memory buffer.
 * @param buf Pointer to constant buffer.
 * @param len Length of buffer.
 * @param managed If true, stream will free buffer on STM_close.
 * @return Initialized STM_Stream_T.
 */
STM_Stream_T STM_fromConstBuf(const uint8_t* buf, size_t len, bool managed);

/**
 * @brief Create a stream for standard input (stdin).
 * @return STM_Stream_T for stdin.
 */
STM_Stream_T STM_stdin(void);

/**
 * @brief Create a stream for standard output (stdout).
 * @return STM_Stream_T for stdout.
 */
STM_Stream_T STM_stdout(void);

/**
 * @brief Close a stream and release its resources if managed.
 * @param stream Pointer to stream to close.
 * @param err Pointer to error structure to receive error info.
 */
void STM_close(STM_Stream_T* stream, STM_Err_T* err);

/**
 * @brief Read a single byte from a stream.
 * @param stream Pointer to stream.
 * @param err Pointer to error structure to receive error info.
 * @return Byte read, or 0 on error/end of stream.
 */
uint8_t STM_get(STM_Stream_T* stream, STM_Err_T* err);

/**
 * @brief Write a single byte to a stream.
 * @param stream Pointer to stream.
 * @param byte Byte to write.
 * @param err Pointer to error structure to receive error info.
 */
void STM_put(STM_Stream_T* stream, uint8_t byte, STM_Err_T* err);

/**
 * @brief Read multiple bytes from a stream into a buffer.
 * @param stream Pointer to stream.
 * @param buf Buffer to read into.
 * @param len Number of bytes to read.
 * @param err Pointer to error structure to receive error info.
 * @return Number of bytes actually read.
 */
size_t STM_read(STM_Stream_T* stream, void* buf, size_t len, STM_Err_T* err);

/**
 * @brief Write multiple bytes from a buffer to a stream.
 * @param stream Pointer to stream.
 * @param buf Buffer to write from.
 * @param len Number of bytes to write.
 * @param err Pointer to error structure to receive error info.
 * @return Number of bytes actually written.
 */
size_t STM_write(STM_Stream_T* stream, const void* buf, size_t len, STM_Err_T* err);

/**
 * @brief Write formatted output to a stream (like printf).
 * @param stream Pointer to stream.
 * @param err Pointer to error structure to receive error info.
 * @param str Format string.
 * @param ... Additional arguments for formatting.
 * @return Number of characters written.
 */
size_t STM_printf(STM_Stream_T* stream, STM_Err_T* err, const char* str, ...);

/**
 * @brief Get a human-readable error message for a stream error.
 * @param err Error structure.
 * @return Error message string.
 */
const char* STM_getErrMsg(STM_Err_T err);

