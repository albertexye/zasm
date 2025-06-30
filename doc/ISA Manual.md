# Overview
---
This is an educational project for me to learn about basic computer architecture, digital circuit, and low-level programming.

# Architecture
---
## Architecture Overview

The computer consists of a clock, multiple registers, and an ALU. It has an 8-bit DIP switch as the input and a 2-digit hexadecimal number display as the output.

## Registers

| Name | Mode | Purpose                                                        |
| ---- | ---- | -------------------------------------------------------------- |
| A    | RW   | The address register controls the address of the SRAM module   |
| C    | RW   | The conditional register controls whether the PC loads a value |
| G    | RW   | A general purpose register                                     |
| X    | RW   | The X register is the first operand of the ALU                 |
| Y    | RW   | The Y register is the second operand of the ALU                |
| N    | WO   | The register for the number display                            |
| P    | RW*  | The PC                                                         |


Note: Although the PC is a read-write register, a write only takes place if the C register is in a certain condition. See below for more information.

Every writable register has a WE pin, and every readable register has an ~OE pin.

## ALU

The ALU can do addition and subtraction between X and Y. The SUB pin indicates which operation to perform. When in addition mode, it outputs X + Y; when in subtraction mode, it outputs X - Y.

The ALU has an ~OE pin.

## SRAM

The SRAM module's address lines are connected to the internal value of register A. It has a WE pin and an ~OE pin.
