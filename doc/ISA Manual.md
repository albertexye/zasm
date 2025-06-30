# ZASM 8-bit Computer: Instruction Set Architecture (ISA) Manual

## Table of Contents

1. [Introduction](#1-introduction)
2. [Architecture Overview](#2-architecture-overview)
3. [Registers](#3-registers)
4. [Memory Map & I/O](#4-memory-map--io)
5. [Instruction Format](#5-instruction-format)
6. [Addressing Modes](#6-addressing-modes)
7. [Instruction Set Reference](#7-instruction-set-reference)
8. [Assembly Syntax](#8-assembly-syntax)
9. [Example Programs](#9-example-programs)
10. [Appendix: Control Signals & Pinout](#10-appendix-control-signals--pinout)

## 1. Introduction

This manual documents the instruction set, register map, and programming model for the ZASM 8-bit computer. It is intended for users writing assembly code, building tools, or interfacing with the hardware.

## 2. Architecture Overview

- **Data Width:** 8 bits
- **Registers:** General-purpose and special registers (see below)
- **ALU:** 8-bit addition and subtraction
- **Memory:** 256 bytes SRAM, 256 bytes ROM (Harvard architecture)
- **I/O:** 8-bit DIP switch input, 2-digit hexadecimal display output

## 3. Registers

| Name | Code | Description                |
|------|------|----------------------------|
| A    |  a   | Address register           |
| C    |  c   | Conditional register       |
| G    |  g   | General-purpose register   |
| M    |  m   | Memory (at address in A)   |
| X    |  x   | Operand 1                  |
| Y    |  y   | Operand 2                  |
| N    |  n   | Number (write-only, display output) |
| P    |  p   | Program counter            |
| B    |  b   | Button input (DIP switch)  |
| D    |  d   | Difference (X - Y)         |
| J    |  j   | Jump condition (C != 0)    |
| L    |  l   | Left shift (A << 4)        |
| S    |  s   | Sum (X + Y)                |
| Z    |  z   | Zero (read-only, always 0) |

**Special behaviors:**

- **N** is write-only; writing sets the 2-digit 7-segment display. Reading always returns 0.
- **B** always reflects the state of the DIP switch input.
- **Z** is a read-only pseudo-register; reading it always returns 0 (implemented by disabling all bus outputs).
- **RST** resets all registers except the PC. This is intentional to avoid infinite loops on power-up.

## 4. Memory Map & I/O

- **Harvard architecture:**
  - **Program ROM:** 0x00–0xFF (256 bytes)
  - **Data RAM:** 0x00–0xFF (256 bytes)
- **I/O:**
  - **Input:** B register (DIP switch, 8 bits)
  - **Output:** N register (write-only, 2-digit 7-segment display)

## 5. Instruction Format

Each instruction is 1 byte (8 bits).

- **MOV:** \[dest:4\]\[src:4\] (also used for JNZ)
- **LDI:** 1\[reg:3\]\[imm:4\] (top bit set)
- **JEZ:** 0\[reg:3\]\[1\]\[1\]\[1\]\[x\] (0b0xxx1110/0b0xxx1111, see below)
- **JNI:** 1111\[imm:4\] (top nibble 0xF)
- **HLT:** 01101111
- **RST:** 01111111

**Encoding details:**

- **MOV/JNZ:**
  - MOV r1 r2: r1 ← r2
  - JNZ is encoded as MOV P R (P = PC, R = source register)
- **JEZ:**
  - Encoded as 0xxx1110 or 0xxx1111. The last bit is the MSB of the register code, and "xxx" are the lower 3 bits. This forms a 4-bit register code.
- **JNI:**
  - Encoded as 1111iiii (top nibble 0xF), where iiii is the immediate value.
- **HLT/RST:**
  - HLT: 01101111
  - RST: 01111111

## 6. Addressing Modes

- **Register Direct:** Most instructions operate directly on registers.
- **Immediate:** LDI and JNI use 4-bit immediate values.
- **Memory Indirect:** Access via A register (M = mem[A]).

## 7. Instruction Set Reference

| Mnemonic | Format         | Description                        |
|----------|---------------|------------------------------------|
| mov r1 r2| r1, r2        | r1 ← r2                            |
| ldi r i  | r, imm        | r ← imm (4 bits)                   |
| jez r    | r             | if (C == 0) PC ← r                 |
| jnz r    | r             | if (C != 0) PC ← r                 |
| jni i    | imm           | if (C != 0) PC ← imm (4 bits)      |
| hlt      | —             | Halt execution                     |
| rst      | —             | Reset all registers except PC       |

**Instruction Details:**

- **mov r1 r2:** Move value from r2 to r1.
- **ldi r i:** Load immediate value i into register r (4 bits only).
- **jez r:** Jump to address in r if C == 0. (Special encoding, see above.)
- **jnz r:** Jump to address in r if C != 0. (Implemented as MOV P r.)
- **jni i:** Jump to immediate address i if C != 0. (Implemented as ldi p, i.)
- **hlt:** Halt the CPU.
- **rst:** Reset all registers except PC.

**Special behaviors:**

- **No unconditional jump:** To jump unconditionally, set C to 0 and use `jez c`.
- **No JEI:** There is no "jump if equal immediate" due to encoding space.
- **Loading 8-bit immediates:** To load an 8-bit value, load a 4-bit value, left shift, load another 4-bit value, and add.
- **Assembly is case-insensitive.**

## 8. Assembly Syntax

- **Comments:** Use `;` for comments.
- **Case-insensitive:** All mnemonics and register names are case-insensitive.
- **Example:**

  ```zasm
  rst         ; reset
  ldi c 1     ; set conditional
  ldi y 1     ; add one at a time
  mov n x     ; show x
  mov x s     ; increment x
  jni 3       ; infinite loop
  ```

## 9. Example Programs

See `asm/` for sample programs.

## 10. Appendix: Control Signals & Pinout

See `EEPROM Pin Configuration.md` for detailed pinout and control signal mapping.
