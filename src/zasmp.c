/**
 * @file zasmp.c
 * @brief Implementation of Zero Page (ZP) packing and conversion utilities for ZASM.
 *
 * This file provides the implementation for transforming 256-byte blocks between different
 * ZP target formats (instruction, number, microcode) as used in the assembler. The static
 * functions below perform the core byte mapping and transformation logic.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "stream.h"
#include "zasm.h"
#include "zasmp.h"

// --- Internal types ---
typedef struct {
  uint8_t p0: 3;
  uint8_t p1: 3;
  uint8_t p2: 3;
  uint8_t p3: 3;
  uint8_t p4: 3;
  uint8_t p5: 3;
  uint8_t p6: 3;
  uint8_t p7: 3;
} ZP_Convert_T;

/**
 * @brief Converts a byte using the specified bit mapping table.
 *
 * @param table Bit mapping table to use for conversion.
 * @param byte  Input byte to convert.
 * @return Converted byte with bits rearranged according to the table.
 */
static uint8_t ZP_mapByte(const ZP_Convert_T table, const uint8_t byte);

/**
 * @brief Applies a bit mapping table to all 256 bytes in the array.
 *
 * @param table Bit mapping table to use for conversion.
 * @param bytes Array of 256 bytes to be mapped in-place.
 */
static void ZP_mapBytes(const ZP_Convert_T table, uint8_t* const bytes);

static void ZP_reverseBytes(uint8_t* bytes);

static const ZP_Convert_T s_ZP_InstTable = {
  7, 6, 5, 4, 3, 2, 1, 0
};

static const ZP_Convert_T s_ZP_Number = {
  1, 2, 3, 0, 4, 5, 6, 7
};

static const ZP_Convert_T s_ZP_Microcode = {
  0, 1, 2, 7, 6, 5, 4, 3
};

static uint8_t ZP_mapByte(const ZP_Convert_T table, const uint8_t byte) {
  uint8_t ret = 0;
  // Map each bit according to the table
  ret |= ((byte >> table.p0) & 1) << 0;
  ret |= ((byte >> table.p1) & 1) << 1;
  ret |= ((byte >> table.p2) & 1) << 2;
  ret |= ((byte >> table.p3) & 1) << 3;
  ret |= ((byte >> table.p4) & 1) << 4;
  ret |= ((byte >> table.p5) & 1) << 5;
  ret |= ((byte >> table.p6) & 1) << 6;
  ret |= ((byte >> table.p7) & 1) << 7;
  return ret;
}

static void ZP_mapBytes(const ZP_Convert_T table, uint8_t* const bytes) {
  // Apply mapping to each byte in the array
  for (size_t i = 0; i < 256; ++i) {
    bytes[i] = ZP_mapByte(table, bytes[i]);
  }
}

static void ZP_reverseBytes(uint8_t* const bytes) {
  // Flip the binary representation of each address
  uint8_t buf[256];
  for (size_t i = 0; i < 256; ++i) {
    uint8_t addr = 0;
    for (size_t j = 0; j < 8; ++j) {
      addr |= ((i >> j) & 1) << (7 - j);
    }
    buf[addr] = bytes[i];
  }
  // Copy the reversed bytes back to the original array
  memcpy(bytes, buf, 256);
}

void ZP_pack(STM_Stream_T *const in, STM_Stream_T *const out, ZP_Target_E target, STM_Err_T *const err) {
  uint8_t bytes[256] = {0};
  STM_read(in, bytes, 256, err);
  if (err->err != STM_ERR_OK && err->err != STM_ERR_EOF) return;
  err->err = STM_ERR_OK; // Reset error to OK after reading
  switch (target) {
    case ZP_TARGET_INST:
      ZP_mapBytes(s_ZP_InstTable, bytes);
      break;
    case ZP_TARGET_NUMBER:
      ZP_mapBytes(s_ZP_Number, bytes);
      ZP_reverseBytes(bytes);
      break;
    case ZP_TARGET_MICROCODE:
      ZP_mapBytes(s_ZP_Microcode, bytes);
      ZP_reverseBytes(bytes);
      break;
    default:
      assert(false);
  }
  STM_write(out, bytes, 256, err);
}

#if ZA_TGT == ZA_TGT_P
#include "zasmcli.h"

int main(const int argc, char *argv[]) {
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_IN, "in", {.stream = {0}}},
    {ZCLI_ARG_STREAM_OUT, "out", {.stream = {0}}},
    {ZCLI_ARG_CHAR, "mode", {.c = 0}},
  };
  ZCLI_ArgList_T argList = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&argList, argc, argv);
  ZP_Target_E target;
  switch (args[2].value.c) {
    case 'i':
      target = ZP_TARGET_INST;
      break;
    case 'n':
      target = ZP_TARGET_NUMBER;
      break;
    case 'm':
      target = ZP_TARGET_MICROCODE;
      break;
    default:
      ZCLI_error("invalid mode, must be 'i', 'n', or 'm'");
      ZCLI_freeArgs(&argList);
      return 1;
  }
  STM_Err_T err = {0};
  ZP_pack(&args[0].value.stream, &args[1].value.stream, target, &err);
  ZCLI_freeArgs(&argList);
  if (err.err != STM_ERR_OK && err.err != STM_ERR_EOF) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  return 0;
}
#endif

