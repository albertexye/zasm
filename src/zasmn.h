/**
 * @file zasmn.h
 * @brief Number table generator for ZASM project.
 *
 * This header provides the interface for generating a number table used in the ZASM assembler project.
 * The main function, ZN_generate, outputs a 256-byte table based on a 7-segment display encoding.
 *
 * Usage:
 *   - Call ZN_generate to write the number table to a stream.
 *   - The 'page' parameter selects which nibble (high or low) to use for encoding.
 *   - Error handling is performed via the STM_Err_T structure.
 */

#pragma once

#include "stream.h"

/**
 * @brief Generate a 256-byte number table for 7-segment display encoding.
 *
 * This function writes a 256-byte table to the provided output stream. Each byte represents
 * the 7-segment encoding for a nibble of the input value. The 'page' parameter selects whether
 * to use the high nibble (page = 1) or low nibble (page = 0) for encoding.
 *
 * @param[out] out  Output stream to write the table to.
 * @param[in]  page Selects nibble: 0 for low nibble, 1 for high nibble.
 * @param[out] err  Error structure; set if an error occurs during writing.
 */
void ZN_generate(STM_Stream_T* out, bool page, STM_Err_T* err);

