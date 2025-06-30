/**
 * @file zasmf.c
 * @brief Implementation of ZASM Flash Tool device communication and protocol logic.
 *
 * This file contains the implementation of the ZASM flash protocol, including serial port
 * configuration, protocol parsing, CRC calculation, and device communication logic. It provides
 * static helper functions for protocol state management, error handling, and low-level I/O.
 *
 * Only static (internal) functions are documented here. Exported API functions are documented in the header.
 */

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

/**
 * @brief Configure a termios structure for 115200 8N1, no flow control, no parity.
 * @param tty Pointer to the termios structure to configure.
 */
static void ZF_setTty(struct termios* tty);

/**
 * @brief Update CRC16 with a new byte.
 * @param byte Input byte.
 * @param hash Current CRC value.
 * @return Updated CRC value.
 */
static uint16_t ZF_crc16Update(uint8_t byte, uint16_t hash);

/**
 * @brief Finalize CRC16 calculation.
 * @param hash Current CRC value.
 * @return Final CRC value.
 */
static uint16_t ZF_crc16Finalize(uint16_t hash);

/**
 * @brief Compute CRC16 for a data buffer.
 * @param data Pointer to data buffer.
 * @param len Length of data in bytes.
 * @return CRC16 value.
 */
static uint16_t ZF_crc16(const uint8_t* data, uint16_t len);

/**
 * @brief Read from the device into the context buffer.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_readTty(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Clear the context buffer and reset protocol state.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_clearTty(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Get the next byte from the context buffer.
 * @param ctx Pointer to the ZASM context.
 * @return Next byte from buffer.
 */
static uint8_t ZF_getc(ZF_Ctx_T* ctx);

/**
 * @brief Parse the protocol magic byte from the buffer.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_parseMagic(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Parse the protocol operation byte from the buffer.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_parseOp(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Parse the protocol hash (CRC) from the buffer and validate.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_parseHash(ZF_Ctx_T* ctx, ZF_Err_T* err);

/**
 * @brief Sleep for approximately 1 millisecond.
 * @param err Pointer to error structure.
 */
static void ZF_millisleep(ZF_Err_T* err);

/**
 * @brief Get the current time in microseconds.
 * @param err Pointer to error structure.
 * @return Time in microseconds since epoch.
 */
static long ZF_macroTime(ZF_Err_T* err);

/**
 * @brief Append a byte to the context buffer.
 * @param ctx Pointer to the ZASM context.
 * @param byte Byte to append.
 */
static void ZF_putc(ZF_Ctx_T* ctx, uint8_t byte);

/**
 * @brief Write the context buffer to the device, appending CRC.
 * @param ctx Pointer to the ZASM context.
 * @param err Pointer to error structure.
 */
static void ZF_writeTty(ZF_Ctx_T* ctx, ZF_Err_T* err);

typedef enum : uint8_t {
  ZF_OP_PING,      // ping the device
  ZF_OP_SEND_LOW,  // write data to the lower 256 bytes
  ZF_OP_SEND_HIGH, // write data to the higher 256 bytes
  ZF_OP_ACK,       // acknowledge a packet
} ZF_Op_E;

static const char* const s_ZF_ErrMsg[] = {
  "invalid start byte",
  "invalid operation",
  "invalid hash",
};


static void ZF_setTty(struct termios* const tty) {
  // 115200, 8N1, 1 stop bit, no flow control, no parity
  // Set input and output baud rates to 115200
  cfsetospeed(tty, B115200);
  cfsetispeed(tty, B115200);

  // Control Modes (c_cflag)
  // CS8     - 8 bits per byte
  // CREAD   - Enable receiver
  // CLOCAL  - Ignore modem control lines
  tty->c_cflag |= (CS8 | CREAD | CLOCAL);

  // Turn off parity
  tty->c_cflag &= (unsigned)(~PARENB);
  tty->c_cflag &= (unsigned)(~CSTOPB); // 1 stop bit

  // Local Modes (c_lflag)
  // ICANON  - Disable canonical mode (input available byte-by-byte)
  // ECHO    - Disable echo
  // ECHOE   - Disable ERASE as backspace
  // ECHONL  - Disable newline echo
  // ISIG    - Disable interpretation of INTR, QUIT, SUSP characters
  tty->c_lflag &= (unsigned)(~(ICANON | ECHO | ECHOE | ECHONL | ISIG));

  // Input Modes (c_iflag)
  // IXON    - Disable XON/XOFF flow control on output
  // IXOFF   - Disable XON/XOFF flow control on input
  // IXANY   - Disable any character will restart output (used with IXON)
  // IGNBRK  - Ignore break conditions
  // BRKINT  - Disable break causes interrupt
  // PARMRK  - Disable mark parity errors
  // ISTRIP  - Disable strip off 8th bit
  // INLCR   - Disable translate NL to CR
  // IGNCR   - Disable ignore CR
  // ICRNL   - Disable translate CR to NL
  tty->c_iflag &= (unsigned)(~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL));

  // Output Modes (c_oflag)
  // OPOST   - Disable post-processing of output (raw output)
  tty->c_oflag &= (unsigned)(~OPOST);

  // Control Characters (c_cc)
  // VMIN    - Minimum number of characters for non-canonical read
  // VTIME   - Timeout in deciseconds for non-canonical read
  tty->c_cc[VMIN] = 0;  // Return immediately
  tty->c_cc[VTIME] = 0; // No timeout (wait indefinitely for VMIN characters)
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
  if (tcflush(ctx.fd, TCOFLUSH) != 0) goto error;
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
  for (uint16_t i = (1 << 15), j = 1; i; i >>= 1, j <<= 1)
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
  if (tcflush(ctx->fd, TCIFLUSH | TCOFLUSH) == -1) {
    err->err = ZF_ERR_ERRNO;
    err->code.errno_ = errno;
  }
}

static uint8_t ZF_getc(ZF_Ctx_T* const ctx) {
  assert(ctx->bufPos < ctx->bufLen);
  return ctx->buf[ctx->bufPos++];
}

static void ZF_parseMagic(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  if (ZF_getc(ctx) == 0xAA) {
    ctx->rcvState = ZF_RCV_STATE_OP;
    return;
  }
  err->err = ZF_ERR_PROTOCOL;
  err->code.protocol = ZF_PROTOCOL_ERR_START_BYTE;
}

static void ZF_parseOp(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const uint8_t op = ZF_getc(ctx);
  if (op == ZF_OP_ACK) {
    ctx->rcvState = ZF_RCV_STATE_CRC;
    return;
  }
  err->err = ZF_ERR_PROTOCOL;
  err->code.protocol = ZF_PROTOCOL_ERR_OP;
}

static void ZF_parseHash(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  const uint16_t crc = (uint16_t)(((uint16_t)ZF_getc(ctx) << 8) | (uint16_t)ZF_getc(ctx));
  const uint16_t hash = ZF_crc16(ctx->buf, ctx->bufPos - 2);
  if (crc != hash) {
    err->err = ZF_ERR_PROTOCOL;
    err->code.protocol = ZF_PROTOCOL_ERR_CRC;
  }
  ZF_clearTty(ctx, err);
}

bool ZF_poll(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  ZF_readTty(ctx, err);
  while (true) {
    if (err->err != ZF_ERR_OK) break;
    const size_t avail = ctx->bufLen - ctx->bufPos;
    if (avail == 0) return false;
    switch (ctx->rcvState) {
      case ZF_RCV_STATE_NONE:
        ZF_parseMagic(ctx, err);
        break;
      case ZF_RCV_STATE_OP:
        ZF_parseOp(ctx, err);
        break;
      case ZF_RCV_STATE_CRC:
        if (avail < 2) return false;
        ZF_parseHash(ctx, err);
        return err->err == ZF_ERR_OK;
      default:
        assert(false);
    }
  }
  ZF_clearTty(ctx, err);
  return false;
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

bool ZF_block(
  ZF_Ctx_T* const ctx,
  const uint32_t timeout,
  ZF_Err_T* const err
) {
  const long start = ZF_macroTime(err);
  if (err->err != ZF_ERR_OK) return false;
  while (true) {
    const bool ack = ZF_poll(ctx, err);
    if (err->err != ZF_ERR_OK || ack) return ack;
    const long now = ZF_macroTime(err);
    if (err->err != ZF_ERR_OK) return false;
    if (now - start >= timeout * 1000) return false;
    ZF_millisleep(err);
    if (err->err != ZF_ERR_OK) return false;
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
    ZF_Err_T err_;
    ZF_clearTty(ctx, &err_);
    return;
  }
  ZF_clearTty(ctx, err);
}

void ZF_ping(ZF_Ctx_T* const ctx, ZF_Err_T* const err) {
  ZF_putc(ctx, 0xAA); // magic
  ZF_putc(ctx, ZF_OP_PING);
  ZF_writeTty(ctx, err);
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

#if ZA_TGT == ZA_TGT_F
#include <stdlib.h>

#include "zasmcli.h"
#include "stream.h"

static const char *const s_ZF_Usage = "zasmf [device]";

static const char *const s_ZF_HelpMsg =
  "zasmf - ZASM flash tool\n"
  "Commands:\n"
  "  p - ping the device\n"
  "  w - write data to the device\n"
  "  q - quit\n"
  "  h - help (this message)\n";

static bool ZF_inputPage(bool *const page) {
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
  printf("quit\n");
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

static void ZF_exec(ZF_Ctx_T *const ctx, const char cmd) {
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
    case 'w':
      if (!ZF_inputPage(&page)) return;
      if (!ZF_readFile(buf)) return;
      printf("write\n");
      ZF_write(ctx, buf, page, &err);
      break;
    case 'h':
      printf(s_ZF_HelpMsg);
      return;
    default:
      ZCLI_error("unrecognized command - 'h' for help");
      return;
  }
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return;
  }
  const bool ack = ZF_block(ctx, 1000, &err);
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return;
  }
  if (ack) printf("acknowledged\n");
  else ZCLI_error("timeout");
}

[[noreturn]] static void ZF_interactive(ZF_Ctx_T *const ctx) {
  printf("zasmf interactive\n");
  while (true) {
    char cmd;
    printf(">");
    if (!ZCLI_getcmd(&cmd)) {
      ZCLI_error("invalid command - 'h' for help");
      continue;
    }
    ZF_exec(ctx, cmd);
  }
}

int main(const int argc, char *const argv[]) {
  if (argc != 2) {
    ZCLI_error("bad arguments");
    printf("usage: %s\n", s_ZF_Usage);
    return 1;
  }
  ZF_Err_T err;
  ZF_Ctx_T ctx = ZF_open(argv[1], &err);
  if (err.err != ZF_ERR_OK) {
    ZCLI_error(ZF_getErrMsg(err));
    return 1;
  }
  ZF_interactive(&ctx);
}
#endif
