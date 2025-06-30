/**
 * @file zasmf.h
 * @brief ZASM Flash Tool interface for device communication and programming.
 *
 * This header provides the API for communicating with and programming a ZASM-compatible device
 * over a serial interface. It defines error handling, context management, and functions for
 * sending and receiving data, as well as device control operations.
 *
 * All functions and types are designed for use in host-side tools that interact
 * with ZASM devices for flashing, pinging, and protocol management.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @typedef ZF_Fd_T
 * @brief File descriptor type for device communication.
 */
typedef int ZF_Fd_T;

/**
 * @enum ZF_Err_E
 * @brief Error codes for ZASM flash operations.
 */
typedef enum : uint8_t {
  ZF_ERR_OK,         /**< No error. */
  ZF_ERR_PROTOCOL,   /**< Protocol error. See ZF_ProtocolErr_E. */
  ZF_ERR_OOM,        /**< Out of memory. */
  ZF_ERR_ERRNO,      /**< System error (see errno). */
} ZF_Err_E;

/**
 * @enum ZF_ProtocolErr_E
 * @brief Protocol-specific error codes.
 */
typedef enum : uint8_t {
  ZF_PROTOCOL_ERR_START_BYTE, /**< Invalid start byte received. */
  ZF_PROTOCOL_ERR_OP,         /**< Invalid operation code received. */
  ZF_PROTOCOL_ERR_CRC,       /**< Invalid CRC received. */
} ZF_ProtocolErr_E;

/**
 * @struct ZF_Err_T
 * @brief Error structure for ZASM flash operations.
 *
 * Contains the error code and additional protocol or system error information.
 */
typedef struct {
  ZF_Err_E err; /**< Main error code. */
  union {
    ZF_ProtocolErr_E protocol; /**< Protocol error code (if err == ZF_ERR_PROTOCOL). */
    int errno_;                /**< System error code (if err == ZF_ERR_ERRNO). */
  } code;
} ZF_Err_T;

/**
 * @enum ZF_Rcv_State_E
 * @brief Receive state for protocol parsing.
 */
typedef enum : uint8_t {
  ZF_RCV_STATE_NONE, /**< No data received. */
  ZF_RCV_STATE_OP,   /**< Operation byte expected. */
  ZF_RCV_STATE_CRC, /**< CRC expected. */
} ZF_Rcv_State_E;

/**
 * @struct ZF_Ctx_T
 * @brief Context for ZASM flash communication.
 *
 * Holds the file descriptor, receive state, buffer, and buffer positions for protocol operations.
 */
typedef struct {
  ZF_Fd_T fd;             /**< File descriptor for the device. */
  ZF_Rcv_State_E rcvState;/**< Current receive state. */
  uint16_t bufPos;        /**< Current buffer parsing position. */
  uint16_t bufLen;        /**< Current buffer length. */
  uint8_t* buf;           /**< Pointer to the data buffer. */
} ZF_Ctx_T;

/**
 * @brief Open a device for ZASM flash communication.
 *
 * @param device Path to the device (e.g., "/dev/ttyUSB0").
 * @param err Pointer to error structure to receive error information.
 * @return Initialized ZF_Ctx_T context. On error, context fields are zeroed and err is set.
 */
ZF_Ctx_T ZF_open(const char* device, ZF_Err_T* err);

/**
 * @brief Close a ZASM flash context and release resources.
 *
 * @param ctx Pointer to the context to close.
 * @param err Pointer to error structure to receive error information.
 */
void ZF_close(ZF_Ctx_T* ctx, ZF_Err_T* const err);

/**
 * @brief Poll the device for incoming data and process protocol state.
 *
 * @param ctx Pointer to the context.
 * @param err Pointer to error structure to receive error information.
 * @return true if an ACK was received, false otherwise.
 */
bool ZF_poll(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Block until an ACK is received or a timeout occurs.
 *
 * @param ctx Pointer to the context.
 * @param timeout Timeout in milliseconds.
 * @param err Pointer to error structure to receive error information.
 * @return true if ACK received before timeout, false otherwise.
 */
bool ZF_block(ZF_Ctx_T* ctx, uint32_t timeout, ZF_Err_T* err);

/**
 * @brief Send a ping command to the device.
 *
 * @param ctx Pointer to the context.
 * @param err Pointer to error structure to receive error information.
 */
void ZF_ping(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Write a page of data to the device.
 *
 * @param ctx Pointer to the context.
 * @param data Pointer to the data to write (256 bytes).
 * @param page Page selector (true for high page, false for low page).
 * @param err Pointer to error structure to receive error information.
 */
void ZF_write(ZF_Ctx_T* ctx, const void* data, bool page, ZF_Err_T* err);

/**
 * @brief Get a human-readable error message for a ZF_Err_T error.
 *
 * @param err Error structure.
 * @return Pointer to a string describing the error.
 */
const char *ZF_getErrMsg(ZF_Err_T err);
