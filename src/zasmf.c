#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "zasm.h"
#include "zasmf.h"

#define ZF_BUF_SIZE 260

typedef enum : uint8_t {
  ZF_OP_PING,      // ping the device
  ZF_OP_SEND_LOW,  // write data to the lower 256 bytes
  ZF_OP_SEND_HIGH, // write data to the higher 256 bytes
  ZF_OP_READ_LOW,  // read data from the lower 256 bytes
  ZF_OP_READ_HIGH, // read data from the higher 256 bytes
  ZF_OP_ACK,       // acknowledge a packet
} ZF_Op_E;

static const char* const s_ZF_ErrMsg[] = {
  "",
  "error opening ttf port",
  "error writing to ttf",
  "error reading from ttf",
  "protocol error",
  "out of memory",
};

static void ZF_setTty(struct termios* const tty) {
  // 115200, 8N1, 1 stop bit, no flow control, no parity
  tty->c_ispeed = B115200;
  tty->c_ospeed = B115200;
  tty->c_cflag &= (tcflag_t)(~PARENB); // no parity
  tty->c_cflag &= (tcflag_t)(~CSTOPB); // 1 stop bit
  tty->c_cflag &= (tcflag_t)(~CSIZE);  // clear size bits
  tty->c_cflag |= CS8;                 // 8 data bits
  tty->c_cflag &= ~CRTSCTS;            // no flow control
  tty->c_cflag |= CREAD | CLOCAL; // enable receiver, ignore modem control lines
  tty->c_iflag &=
    (tcflag_t)(~(IXON | IXOFF | IXANY)); // no software flow control
  tty->c_lflag &= (tcflag_t)(~(ICANON | ECHO | ECHOE | ISIG)); // raw input
  tty->c_oflag &= (tcflag_t)(~OPOST);                          // raw output
  tty->c_cc[VMIN] = 0;
  tty->c_cc[VTIME] = 0;
}

ZF_Ctx_T ZF_open(const char* const device, ZF_Err_T* const err) {
  ZF_Ctx_T ctx = {
    .fd = -1,
    .rcvState = ZF_RCV_STATE_NONE,
    .bufPos = 0,
    .bufLen = 0,
    .buf = nullptr
  };
  ctx.buf = malloc(ZF_BUF_SIZE);
  if (!ctx.buf) {
    err->err = ZF_ERR_OOM;
    return ctx;
  }
  ctx.fd = open(device, O_RDWR | O_NONBLOCK | O_NDELAY);
  if (ctx.fd < 0) goto error;
  struct termios tty = {0};
  if (tcgetattr(ctx.fd, &tty) != 0) goto error;
  ZF_setTty(&tty);
  if (tcflush(ctx.fd, TCIFLUSH) != 0) goto error;
  if (tcsetattr(ctx.fd, TCSANOW, &tty) != 0) goto error;
  if (fcntl(ctx.fd, F_SETFL, O_RDWR | O_NOCTTY | O_NDELAY) == -1) goto error;
  err->err = ZF_ERR_OK;
  return ctx;
error:
  err->err = ZF_ERR_ERRNO;
  err->code.errno_ = errno;
  ZF_Err_T errClose;
  ZF_close(&ctx, &errClose);
  return ctx;
}

void ZF_close(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  if (ctx->fd >= 0) {
    if (close(ctx->fd) == -1) {
      err->err = ZF_ERR_ERRNO;
      err->code.errno_ = errno;
    }
    ctx->fd = -1;
  }
  free(ctx->buf);
  ctx->buf = nullptr;
  ctx->rcvState = ZF_RCV_STATE_NONE;
  ctx->bufPos = 0;
  ctx->bufLen = 0;
}

static uint16_t ZF_crc16Update(const uint8_t byte, uint16_t hash) {
  for (size_t i = 0; i < 8; ++i) {
    const bool bitFlag = hash >> 15;
    hash <<= 1;
    hash |= (byte >> i) & 1;
    if (bitFlag) hash ^= 0x8005;
  }
  return hash;
}

static uint16_t ZF_crc16Finalize(uint16_t hash) {
  hash = ZF_crc16Update(0, hash);
  hash = ZF_crc16Update(0, hash);
  uint16_t crc = 0;
  for (int i = (1 << 15), j = 1; i; i >>= 1, j <<= 1)
    if (i & hash) crc |= j;
  return crc;
}

static uint16_t ZF_crc16(const uint8_t* const data, const uint16_t len) {
  uint16_t hash = 0;
  for (uint16_t i = 0; i < len; ++i)
    hash = ZF_crc16Update(data[i], hash);
  return ZF_crc16Finalize(hash);
}

static void ZF_readTty(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const ssize_t size = read(
    ctx->fd, ctx->buf + ctx->bufLen, ZF_BUF_SIZE - ctx->bufLen);
  if (size == -1) {
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
    return;
  }
  ctx->bufLen += size;
  return;
}

static void ZF_clearTty(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  ctx->bufPos = 0;
  ctx->bufLen = 0;
  ctx->rcvState = ZF_RCV_STATE_NONE;
  if (tcflush(ctx->fd, TCIFLUSH) == -1) {
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
  }
}

static uint8_t ZF_getc(ZF_Ctx_T* const ctx) {
  assert(ctx->bufPos < ctx->bufLen);
  return ctx->buf[ctx->bufPos++];
}

static void ZF_parseMagic(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  if (ZF_getc(ctx) != 0xAA) {
    err->err = ZF_ERR_PROTOCOL;
    err->code.protocol = ZF_PROTOCOL_ERR_START_BYTE;
    ZF_clearTty(ctx, err);
    return;
  }
  ctx->rcvState = ZF_RCV_STATE_OP;
}

static void ZF_parseOp(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const uint8_t op = ZF_getc(ctx);
  switch (op) {
    case ZF_OP_ACK:
      ctx->rcvState = ZF_RCV_STATE_HASH;
      break;
    case ZF_OP_SEND_LOW:
      ctx->rcvState = ZF_RCV_STATE_DATA;
      break;
    case ZF_OP_PING:
    case ZF_OP_READ_LOW:
    case ZF_OP_READ_HIGH:
    case ZF_OP_SEND_HIGH:
    default:
      err->err = ZF_ERR_PROTOCOL;
      err->code.protocol = ZF_PROTOCOL_ERR_OP;
      ZF_clearTty(ctx, err);
  }
}

static void ZF_parseData(ZF_Ctx_T* const ctx) {
  ctx->bufPos += 256;
  ctx->rcvState = ZF_RCV_STATE_HASH;
}

static void ZF_parseHash(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const uint16_t crc = (uint16_t)(((uint16_t)ZF_getc(ctx) << 8) | ZF_getc(ctx));
  const uint16_t hash = ZF_crc16(ctx->buf, ctx->bufPos);
  ZF_clearTty(ctx, err);
  if (crc != hash) {
    err->err = ZF_ERR_PROTOCOL;
    err->code.protocol = ZF_PROTOCOL_ERR_HASH;
  }
}

ZF_Evt_T ZF_poll(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  ZF_readTty(ctx, err);
  while (true) {
    if (err->err != ZF_ERR_OK) return ZF_EVT_NONE;
    const size_t avail = ctx->bufLen - ctx->bufPos;
    if (avail == 0) return ZF_EVT_NONE;
    switch (ctx->rcvState) {
      case ZF_RCV_STATE_NONE:
        ZF_parseMagic(ctx, err);
        break;
      case ZF_RCV_STATE_OP:
        ZF_parseOp(ctx, err);
        break;
      case ZF_RCV_STATE_DATA:
        if (avail < 256) return ZF_EVT_NONE;
        ZF_parseData(ctx);
        break;
      case ZF_RCV_STATE_HASH:
        if (avail < 2) return ZF_EVT_NONE;
        ZF_parseHash(ctx, err);
        if (err->err != ZF_ERR_OK) return ZF_EVT_NONE;
        goto end;
      default:
        assert(false);
    }
  }
end:
  switch (ctx->buf[1]) {
    case ZF_OP_ACK:
      return ZF_EVT_ACK;
    case ZF_OP_SEND_LOW:
      return ZF_EVT_DATA;
    default:
      assert(false);
  }
}

const void* ZF_getData(ZF_Ctx_T* const ctx) {
  return ctx->buf + 2;
}

static void ZF_millisleep(ZF_Err_T* const err) {
  struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 1'000'000 // 1 ms
  };
  if (nanosleep(&ts, nullptr) == -1) {
    if (errno == EINTR) return; // interrupted, no error
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
  }
}

static long ZF_macroTime(ZF_Err_T* const err) {
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) == -1) {
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
    return 0;
  }
  return tv.tv_sec * 1'000'000 + tv.tv_usec;
}

ZF_Evt_T ZF_block(
  ZF_Ctx_T* const ctx,
  const uint32_t timeout,
  ZF_Err_T* const err
) {
  const long start = ZF_macroTime(err);
  if (err->err != ZF_ERR_OK) return ZF_EVT_NONE;
  while (true) {
    const ZF_Evt_T evt = ZF_poll(ctx, err);
    if (err->err != ZF_ERR_OK || evt != ZF_EVT_NONE) return evt;
    const long now = ZF_macroTime(err);
    if (err->err != ZF_ERR_OK) return ZF_EVT_NONE;
    if (now - start >= timeout * 1000) return ZF_EVT_NONE;
    ZF_millisleep(err);
    if (err->err != ZF_ERR_OK) return ZF_EVT_NONE;
  }
}

static void ZF_putc(ZF_Ctx_T* const ctx, const uint8_t byte) {
  assert(ctx->bufLen < ZF_BUF_SIZE);
  ctx->buf[ctx->bufLen++] = byte;
}

static void ZF_writeTty(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const uint16_t crc = ZF_crc16(ctx->buf, ctx->bufLen);
  ZF_putc(ctx, (uint8_t)(crc >> 8));
  ZF_putc(ctx, (uint8_t)(crc & 0xFF));
  if (write(ctx->fd, ctx->buf, ctx->bufLen) != ctx->bufLen) {
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
    ZF_clearTty(ctx, err);
    return;
  }
  ZF_clearTty(ctx, err);
}

static void ZF_sendOp(ZF_Ctx_T* const ctx, const ZF_Op_E op, ZF_Err_T* const err) {
  ZF_putc(ctx, 0xAA); // magic
  ZF_putc(ctx, op);
  ZF_writeTty(ctx, err);
}

void ZF_ping(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  ZF_sendOp(ctx, ZF_OP_PING, err);
}

void ZF_read(ZF_Ctx_T* const ctx, const bool page, ZF_Err_T* const err) {
  ZF_sendOp(ctx, page ? ZF_OP_READ_HIGH : ZF_OP_READ_LOW, err);
}

void ZF_write(
  ZF_Ctx_T* const ctx,
  const void* const data,
  const bool page,
  ZF_Err_T* const err
) {
  ZF_putc(ctx, 0xAA); // magic
  ZF_putc(ctx, page ? ZF_OP_SEND_HIGH : ZF_OP_SEND_LOW);
  memcpy(ctx->buf + 2, data, 256);
  ctx->bufLen = 258; // 2 bytes for magic and op, 256 for data
  ZF_writeTty(ctx, err);
}

const char* ZF_getErrMsg(const ZF_Err_T err) {
  switch (err.err) {
    case ZF_ERR_OK:
      return "";
    case ZF_ERR_PROTOCOL:
      if (err.code.protocol < sizeof(s_ZF_ErrMsg) / sizeof(s_ZF_ErrMsg[0]))
        return s_ZF_ErrMsg[err.code.protocol];
      else
        return "unknown error";
    case ZF_ERR_OOM:
      return "out of memory";
    case ZF_ERR_ERRNO:
      return strerror(err.code.errno_);
    default:
      return "unknown error";
  }
}

#undef ZA_TGT
#define ZA_TGT ZA_TGT_F

#if ZA_TGT == ZA_TGT_F
#include <stdlib.h>

#include "zasmcli.h"
#include "stream.h"

static const char *const USAGE = "zasmf [device]";

static bool inputPage(bool *const page) {
  printf("page: ");
  char cmd;
  if (!ZCLI_getcmd(&cmd)) {
    ZCLI_error("enter 0 or 1\n");
    return false;
  }
  if (cmd != '0' && cmd != '1') {
    ZCLI_error("enter 0 or 1\n");
    return false;
  }
  *page = cmd == '1';
  return true;
}

[[noreturn]] static void ZF_quit(ZF_Ctx_T *const ctx) {
  ZF_Err_T err = {0};
  ZF_close(ctx, &err);
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    exit(1);
  }
  printf("exit\n");
  exit(0);
}

static bool ZF_readFile(
  uint8_t *const buf
) {
  STM_Stream_T stream;
  if (!ZCLI_inputfile(&stream, "file: ", true))
    return false;
  STM_Err_T serr = {0};
  STM_read(&stream, buf, 256, &serr);
  ZCLI_closefile(&stream);
  if (serr.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(serr));
    return false;
  }
  return true;
}

static bool ZF_writeFile(
  const uint8_t *const buf
) {
  STM_Stream_T stream;
  if (!ZCLI_inputfile(&stream, "file: ", false))
    return false;
  STM_Err_T serr = {0};
  STM_write(&stream, buf, 256, &serr);
  ZCLI_closefile(&stream);
  if (serr.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(serr));
    return false;
  }
  return true;
}

static void exec(ZF_Ctx_T *const ctx, const char cmd) {
  bool page;
  ZF_Err_T err = {0};
  uint8_t buf[256];
  switch (cmd) {
    case 'p':
      printf("ping\n");
      ZF_ping(ctx, &err);
      break;
    case 'q':
      ZF_quit(ctx);
    case 'r':
      if (!inputPage(&page)) return;
      ZF_read(ctx, page, &err);
      break;
    case 'w':
      if (!inputPage(&page)) return;
      if (!ZF_readFile(buf)) return;
      ZF_write(ctx, buf, page, &err);
      break;
    default:
      ZCLI_error("unrecognized command - 'h' for help");
      return;
  }
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return;
  }
  const ZF_Evt_T evt = ZF_block(ctx, 1000, &err);
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return;
  }
  switch (evt) {
    case ZF_EVT_NONE:
      ZCLI_error("timeout");
      return;
    case ZF_EVT_ACK:
      printf("acknowledged\n");
      return;
    case ZF_EVT_DATA:
      ZF_writeFile(ZF_getData(ctx));
      return;
    default:
      assert(false);
  }
}

[[noreturn]] static void interactive(ZF_Ctx_T *const ctx) {
  printf("zasmf interactive\n");
  while (true) {
    char cmd;
    printf(">");
    if (!ZCLI_getcmd(&cmd)) {
      ZCLI_error("invalid command - 'h' for help");
      continue;
    }
    exec(ctx, cmd);
  }
}

int main(const int argc, char *const argv[]) {
  if (argc != 2) {
    ZCLI_error("bad arguments");
    ZCLI_usage(USAGE);
    return 1;
  }
  ZF_Err_T err;
  ZF_Ctx_T ctx = ZF_open(argv[1], &err);
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return 1;
  }
  interactive(&ctx);
}
#endif
