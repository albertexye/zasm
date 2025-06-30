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

- **Build all tools in debug mode (if needed):**

  ```sh
  ./zasmb.sh build d a
  ```

  This will compile the toolchain if sources have changed.

- **Force rebuild all tools in release mode:**

  ```sh
  ./zasmb.sh force_build r a
  ```

  This always recompiles, regardless of timestamps.

- **Run a specific tool in debug mode:**

  ```sh
  ./zasmb.sh run <tool>
  ```

  Replace `<tool>` with one of: `c` (assembler), `d` (disassembler), `f` (flash tool), `m` (microcode), `n` (number table), `p` (page packer), `s` (simulator). For example:

  ```sh
  ./zasmb.sh run c code.s out.bin
  ```

### Tool Overview

- **Assembler (`zasmc`)**: Converts ZASM assembly source files into machine code for the 8-bit computer.
- **Disassembler (`zasmd`)**: Converts machine code back into human-readable assembly.
- **Flash Tool (`zasmf`)**: Communicates with the Arduino Nano programmer to flash EEPROMs via serial.
- **Simulator (`zasms`)**: Runs ZASM programs in a virtual machine for testing and debugging.
- **Microcode Generator (`zasmm`)**: Generates microcode tables and pin mappings for the hardware.
- **Number Table Generator (`zasmn`)**: Produces lookup tables for 7-segment display encoding.
- **Zero Page Packer (`zasmp`)**: Packs and converts 256-byte blocks between different memory formats.

For more details on each tool’s usage and options, run the tool without arguments after building, or consult the source code in the `src/` directory.

## Documentation

- **ISA Manual**: Details the instruction set, register map, and architecture.
- **EEPROM Pin Configuration**: Pinout tables for connecting EEPROM, shift registers, and display hardware.
- **Style Guide**: Coding conventions for contributors.

## Learning Goals

This project is intended for anyone interested in:

- Understanding how computers work at the hardware and software level.
- Learning about instruction sets, assembly language, and toolchain development.
- Experimenting with digital circuits and microcontroller programming.
