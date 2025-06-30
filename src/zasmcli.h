/**
 * @file zasmcli.h
 * @brief Command-line interface utilities for the ZASM assembler project.
 *
 * This header provides functions, macros, and types for handling command-line
 * arguments, file I/O, error reporting, and user interaction in ZASM-based CLI tools.
 * It includes argument parsing, colored output macros, and helpers for binary and stream operations.
 *
 * All functions and types are designed to simplify the development and maintenance
 * of command-line utilities for ZASM and related tools.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "stream.h"

/** \def ZCLI_COLOR_ERROR
 *  @brief ANSI escape code for red (error) text.
 */
#define ZCLI_COLOR_ERROR   "\x1b[31m"
/** \def ZCLI_COLOR_SUCCESS
 *  @brief ANSI escape code for green (success) text.
 */
#define ZCLI_COLOR_SUCCESS "\x1b[32m"
/** \def ZCLI_COLOR_WARNING
 *  @brief ANSI escape code for yellow (warning) text.
 */
#define ZCLI_COLOR_WARNING "\x1b[33m"
/** \def ZCLI_COLOR_IGNORE
 *  @brief ANSI escape code for gray (ignored) text.
 */
#define ZCLI_COLOR_IGNORE  "\x1b[90m"
/** \def ZCLI_COLOR_RESET
 *  @brief ANSI escape code to reset text color.
 */
#define ZCLI_COLOR_RESET   "\x1b[0m"

/**
 * @enum ZCLI_ArgT_E
 * @brief Argument type for command-line parsing.
 */
typedef enum {
  ZCLI_ARG_STREAM_IN,  /**< Input stream argument (file for reading) */
  ZCLI_ARG_STREAM_OUT, /**< Output stream argument (file for writing) */
  ZCLI_ARG_CHAR,       /**< Single character argument */
} ZCLI_ArgT_E;

/**
 * @typedef ZCLI_ArgV_T
 * @brief Value for a command-line argument.
 */
typedef union {
  STM_Stream_T stream; /**< Stream value (input/output file) */
  char c;             /**< Character value */
} ZCLI_ArgV_T;

/**
 * @struct ZCLI_Arg_T
 * @brief Represents a single command-line argument.
 */
typedef struct {
  ZCLI_ArgT_E type;      /**< Argument type */
  const char* name;      /**< Argument name (for usage/help) */
  ZCLI_ArgV_T value;     /**< Argument value */
} ZCLI_Arg_T;

/**
 * @struct ZCLI_ArgList_T
 * @brief List of command-line arguments.
 */
typedef struct {
  ZCLI_Arg_T* args; /**< Array of arguments */
  size_t len;       /**< Number of arguments */
} ZCLI_ArgList_T;

/**
 * @brief Print a formatted error message to stderr (with color).
 * @param msg Error message format string (printf-style).
 * @param ... Arguments for the format string.
 */
void ZCLI_error(const char* msg, ...);

/**
 * @brief Print the current errno as an error message to stderr.
 */
void ZCLI_errno(void);

/**
 * @brief Print a memory region as hexadecimal bytes.
 * @param mem Pointer to memory.
 * @param len Number of bytes to print.
 */
void ZCLI_showmem(const uint8_t* mem, size_t len);

/**
 * @brief Open a file as a stream for reading or writing.
 * @param path Path to the file.
 * @param read True to open for reading, false for writing.
 * @return Opened stream object.
 */
STM_Stream_T ZCLI_openfile(const char* path, bool read);

/**
 * @brief Close a stream and handle errors.
 * @param stream Pointer to the stream to close.
 */
void ZCLI_closefile(STM_Stream_T* stream);

/**
 * @brief Print a byte as binary (8 bits).
 * @param byte Byte to print.
 */
void ZCLI_printbin(uint8_t byte);

/**
 * @brief Read a byte from stdin as binary (8 bits, '0'/'1' chars).
 * @param byte Pointer to store the result.
 * @return True if a valid byte was read, false otherwise.
 */
bool ZCLI_readbin(uint8_t* byte);

/**
 * @brief Clear the input buffer up to and including the next newline.
 */
void ZCLI_clearin(void);

/**
 * @brief Read a single character command from stdin.
 * @param cmd Pointer to store the command character.
 * @return True if a command was read, false otherwise.
 */
bool ZCLI_getcmd(char* cmd);

/**
 * @brief Prompt for a file path and open the file as a stream.
 * @param stream Pointer to store the opened stream.
 * @param prompt Prompt string to display.
 * @param read True to open for reading, false for writing.
 * @return True if the file was opened successfully, false otherwise.
 */
bool ZCLI_inputfile(STM_Stream_T* stream, const char* prompt, bool read);

/**
 * @brief Parse command-line arguments into a ZCLI_ArgList_T structure.
 * @param args Argument list to populate.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 */
void ZCLI_parseArgs(ZCLI_ArgList_T* args, int argc, char* const argv[]);

/**
 * @brief Free resources associated with a ZCLI_ArgList_T (close streams, etc).
 * @param args Argument list to clean up.
 */
void ZCLI_freeArgs(ZCLI_ArgList_T* args);

