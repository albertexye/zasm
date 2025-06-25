#pragma once

#include <stddef.h>
#include <stdint.h>

typedef int ZF_Fd_T;

typedef enum : uint8_t {
  ZF_ERR_OK,
  ZF_ERR_PROTOCOL,
  ZF_ERR_OOM,
  ZF_ERR_ERRNO,
} ZF_Err_E;

typedef enum : uint8_t {
  ZF_PROTOCOL_ERR_START_BYTE,
  ZF_PROTOCOL_ERR_OP,
  ZF_PROTOCOL_ERR_HASH,
} ZF_ProtocolErr_E;

typedef struct {
  ZF_Err_E err;
  union {
    ZF_ProtocolErr_E protocol;
    int errno_;
  } code;
} ZF_Err_T;

typedef enum : uint8_t {
  ZF_RCV_STATE_NONE,
  ZF_RCV_STATE_OP,
  ZF_RCV_STATE_DATA,
  ZF_RCV_STATE_HASH,
} ZF_Rcv_State_E;

typedef struct {
  ZF_Fd_T fd;
  ZF_Rcv_State_E rcvState;
  uint16_t bufPos;
  uint16_t bufLen;
  uint8_t* buf;
} ZF_Ctx_T;

typedef enum : uint8_t {
  ZF_EVT_NONE,
  ZF_EVT_ACK,
  ZF_EVT_DATA,
} ZF_Evt_T;

ZF_Ctx_T ZF_open(const char* device, ZF_Err_T* err);

void ZF_close(ZF_Ctx_T* ctx, ZF_Err_T* const err);

ZF_Evt_T ZF_poll(ZF_Ctx_T* ctx, ZF_Err_T* err);

const void* ZF_getData(ZF_Ctx_T* ctx);

ZF_Evt_T ZF_block(ZF_Ctx_T* ctx, uint32_t timeout, ZF_Err_T* err);

void ZF_ping(ZF_Ctx_T* ctx, ZF_Err_T* err);

void ZF_read(ZF_Ctx_T* ctx, bool page, ZF_Err_T* err);

void ZF_write(ZF_Ctx_T* ctx, const void* data, bool page, ZF_Err_T* err);

const char *ZF_getErrMsg(ZF_Err_T err);
