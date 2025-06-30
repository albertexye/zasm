# ZASM: 8-bit Computer Toolchain & Documentation

ZASM is a toolchain and documentation suite for a home-made 8-bit computer, designed as an educational project to explore basic computer architecture, digital circuits, and low-level programming.

## Project Overview

This project includes:

- **Assembler, Disassembler, and Utilities**: C-based tools for assembling, disassembling, and working with the ZASM instruction set.
- **Firmware**: Arduino Nano code for flashing EEPROMs via a custom serial protocol.
- **Assembly Examples**: Sample assembly programs for the 8-bit computer.
- **Documentation**: Manuals and pin configuration guides for hardware and software.

## Architecture

- **Registers**: The computer features address, conditional, general-purpose, ALU operand, display, and program counter registers.
- **ALU**: Supports 8-bit addition and subtraction.
- **SRAM**: Addressed via the A register, with standard WE and OE controls.
- **I/O**: 8-bit DIP switch input and 2-digit hexadecimal display output.

## Directory Structure

- `src/`: C source code for assembler, disassembler, and related tools.
- `nano/`: Arduino Nano firmware for EEPROM flashing.
- `asm/`: Example assembly code for the 8-bit computer.
- `doc/`: Documentation, including the ISA manual and pin configuration.
- `zasmb.sh`: Build script for compiling and running the toolchain.

## Building & Usage

Use the provided `zasmb.sh` script to build and run the assembler and related tools. See comments in the script for available commands.

## Documentation

- **ISA Manual**: Details the instruction set, register map, and architecture.
- **EEPROM Pin Configuration**: Pinout tables for connecting EEPROM, shift registers, and display hardware.
- **Style Guide**: Coding conventions for contributors.

## Learning Goals

This project is intended for anyone interested in:

- Understanding how computers work at the hardware and software level.
- Learning about instruction sets, assembly language, and toolchain development.
- Experimenting with digital circuits and microcontroller programming.
