/**
 * @file zasmn.c
 * @brief Implementation of number table generation for ZASM project.
 *
 * This file contains the implementation for generating a 256-byte number table used for 7-segment display encoding
 * in the ZASM assembler project. The table is used to map nibbles to their corresponding 7-segment codes.
 *
 * Static functions and data are declared and documented at the top of the file for clarity.
 *
 * Usage:
 *   - Use ZN_generate() to output the number table to a stream (see zasmn.h for details).
 *   - The main() function is only included for the ZA_TGT_N target and provides a CLI interface.
 */

#include <stdint.h>

#include "zasm.h"
#include "zasmn.h"

/**
 * @brief Lookup table for 7-segment display encoding of hexadecimal digits.
 *
 * Each entry corresponds to the 7-segment encoding for a nibble value (0x0 to 0xF).
 */
static uint8_t s_ZP_NumberTable[16] = {
  0b01111110,
  0b00011000,
  0b10110110,
  0b10111100,
  0b11011000,
  0b11101100,
  0b11101110,
  0b00111000,
  0b11111110,
  0b11111100,
  0b11111010,
  0b11001110,
  0b01100110,
  0b10011110,
  0b11100110,
  0b11100010,
};

void ZN_generate(STM_Stream_T* const out, const bool page, STM_Err_T* const err) {
  // Select nibble offset: 0 for low nibble, 4 for high nibble
  const uint8_t off = page ? 4 : 0;
  for (size_t i = 0; i < 256; ++i) {
    // Write the 7-segment encoding for the selected nibble
    STM_put(out, s_ZP_NumberTable[(i >> off) & 0xF], err);
    if (err->err != STM_ERR_OK) return; // Stop on error
  }
}

#if ZA_TGT == ZA_TGT_N
#include "zasmcli.h"

int main(const int argc, char *const argv[]) {
  // Set up CLI arguments: output stream and page selector
  ZCLI_Arg_T args[] = {
    {ZCLI_ARG_STREAM_OUT, "out", {.stream = {0}}},
    {ZCLI_ARG_CHAR, "page", {.c = 0}},
  };
  ZCLI_ArgList_T arglist = {
    .args = args,
    .len = sizeof(args) / sizeof(args[0]),
  };
  ZCLI_parseArgs(&arglist, argc, argv);
  STM_Err_T err = {0};
  const char page = args[1].value.c;
  if (page < '0' || page > '1') {
    ZCLI_error("page must be 0 or 1");
    ZCLI_freeArgs(&arglist);
    return 1;
  }
  // Generate the number table for the selected page
  ZN_generate(&args[0].value.stream, (uint8_t)page - '0', &err);
  ZCLI_freeArgs(&arglist);
  if (err.err != STM_ERR_OK) {
    ZCLI_error(STM_getErrMsg(err));
    return 1;
  }
  return 0;
}
#endif

