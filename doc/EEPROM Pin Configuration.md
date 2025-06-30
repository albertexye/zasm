# ZASM 8-bit Computer: EEPROM Pin Configuration

## Table of Contents

1. [Overview](#1-overview)
2. [Flasher](#2-flasher)
3. [Number Display](#3-number-display)
4. [Instruction](#4-instruction)
5. [Instruction Decoder](#5-instruction-decoder)

## 1. Overview

This file documents the pin connections between the EEPROMs and various parts of the computer.

## 2. Flasher

The flasher is an Arduino Nano board connected to two 74HC595 chips. 74HC595 is an 8-bit shift register used to set the address and data pins for the EEPROM being flashed.

The shift resgiters and EEPROM input pins are connected in a way that the first written bit gets transferred to Pin 0 (IO0 or A0). The least significant bit is written first.

Bit Order:

| Address | IO  | Shift Register |
| ------- | --- | -------------- |
|   A0    | IO0 |       Q7       |
|   A1    | IO1 |       Q6       |
|   A2    | IO2 |       Q5       |
|   A3    | IO3 |       Q4       |
|   A4    | IO4 |       Q3       |
|   A5    | IO5 |       Q2       |
|   A6    | IO6 |       Q1       |
|   A7    | IO7 |       Q0       |

Bits shift from Q0 to Q7.

Pin Diagram:

```text
        ___________________ 
       |                   |
       |       Nano        |
       |_  _  _  _  _  _  _|
       |_D2_D3_D4_D5_D6_D7_|
         |  |  |  |  |  |
   *-----*  |  |  |  |  *--------------*
   |   -----*  |  |  |                 |
   |   |       |  |  *---------------* |
   |   |       |  |                  | |
   |   |    *--*  *-------------*    | |
   |   |    |                   |    | |
   |   *----+--------------*    |    | |
   |   |    |              |    |    | |
   *---+----+----------*   |    |    | |
 _ | _ |  _ | _      _ | _ |  _ | _  | |
|_CLK_SCLK_SER_|    |_CLK_SCLK_SER_| | |
|              |    |              | | |
|     595      |    |     595      | | |
|______ _______|    |______ _______| | |
|______Q_______|    |______Q_______| | |
       |                   |         | |
       *-------*   *-------*         | |
               |   |                 | |
               |   |   *-------------* |
               8   8   |               |
               |   |   |    *----------*
             _ |  _| _ |  _ | _
            |_Addr_IO_Page_~WE_|
            |                  |
            |      EEPROM      |
            |__________________|
```

Note: '*' means the wires are connected, '+' means the wires just cross each other; the number on the wire indicates the number of lines, or 1 if there's no number; '~' means the pin is active-low.

The Page Pin is, in fact, A8. There are 11 address pins, among which 9 are used. The rest is tied to GND.

Pinout Table:

| Pin | Description                  |
| --- | ---------------------------- |
| D2  | Shift register shift clock   |
| D3  | Shift register storage clock |
| D4  | Address shift register data  |
| D5  | Data shift register data     |
| D6  | Page (A8)                    |
| D7  | Write enable                 |

## 3. Number Display

The number display consists of two 7-segment displays side by side, a 555 timer in astable mode, and an EEPROM. The clock output is connected to a demultiplexer and the A8 pin of the EEPROM. The demultiplexer chooses which display to power, and the A8 pin switches the address space to display each digit.

7-Segment Display Pin Definition:

```text
    ___________
  /      2      \
 /\ ___________ /\
/  \           /  \
| 1|           | 3|
\  /___________\  /
 \/      0      \/
 /\ ___________ /\
/  \           /  \
| 6|           | 4|
\  /___________\  /
 \/      5      \/
  \ ___________ /
```

7-Segment Display Digit Table:

| Digit | Code     |
| ----- | -------- |
| 0     | 01111110 |
| 1     | 00011000 |
| 2     | 10110110 |
| 3     | 10111100 |
| 4     | 11011000 |
| 5     | 11101100 |
| 6     | 11101110 |
| 7     | 00111000 |
| 8     | 11111110 |
| 9     | 11111100 |
| A     | 11111010 |
| B     | 11001110 |
| C     | 01100110 |
| D     | 10011110 |
| E     | 11100110 |
| F     | 11100010 |

Note: The first binary digit corresponds to pin 0, and the 7th binary digit corresponds to pin 6. The 8th digit is always 0.

7-Segment Pin Connections:

| EEPROM | 7-Seg |
| ------ | ----- |
| IO0    | 6     |
| IO1    | 5     |
| IO2    | 4     |
| IO3    |       |
| IO4    | 3     |
| IO5    | 2     |
| IO6    | 1     |
| IO7    | 0     |

Bus to 7-Segment Decoder Address Connections:

| EEPROM | Bus |
| ------ | --- |
| A0     | 0   |
| A1     | 1   |
| A2     | 2   |
| A3     | 3   |
| A4     | 4   |
| A5     | 5   |
| A6     | 6   |
| A7     | 7   |

Note: The bus pin 0 is the least significant and physically left-most bit.

## 4. Instruction

The instruction EEPROM stores the instructions.

Instruction to PC/Bus Connections:

| EEPROM | PC |
| ------ | -- |
| A0     | 7  |
| A1     | 6  |
| A2     | 5  |
| A3     | 4  |
| A4     | 3  |
| A5     | 2  |
| A6     | 1  |
| A7     | 0  |

Note: The PC/bus pin 0 is the least significant and physically left-most bit.

## 5. Instruction Decoder

The instruction decoder consists of 3 EEPROMs connected to 24 control lines. The instruction is the address of the 3 EEPROMs.

EEPROM 0 Pin Connections:

| EEPROM | Signal    | Description                       |
| ------ | --------- | --------------------------------- |
| IO0    | ~XO       | X register output enable          |
| IO0    | ~SO (~DO) | ALU output enable                 |
| IO0    | ~YO       | Y register output enable          |
| IO0    | ~AO       | A register output enable          |
| IO0    | ~LO       | Left shift register output enable |
| IO0    | ~BO       | Button input output enable        |
| IO0    | ~MO       | RAM output enable                 |
| IO0    | ~GO       | G register output enable          |

EEPROM 1 Pin Connections:

| EEPROM | Signal | Description                 |
| ------ | ------ | --------------------------- |
| IO0    | ~CO    | C register output enable    |
| IO0    | ~JO    | Jump register output enable |
| IO0    | ~PO    | PC output enable            |
| IO0    | ~IO    | Immediate output enable     |
| IO0    | XI     | X register load enable      |
| IO0    | YI     | Y register load enable      |
| IO0    | AI     | A register load enable      |
| IO0    | MI     | RAM write enable            |

EEPROM 2 Pin Connections:

| EEPROM | Signal | Description                |
| ------ | ------ | -------------------------- |
| IO0    | GI     | G register load enable     |
| IO0    | CI     | C register load enable     |
| IO0    | PI     | PC load enable             |
| IO0    | NI     | Number display load enable |
| IO0    | SB     | ALU sub enable             |
| IO0    | CN     | Conditional negate enable  |
| IO0    | HT     | Halt                       |
| IO0    | ~RS    | PC reset                   |

Note: '~' means the pin is active-low.

Instruction to Instruction Decoder Address Connections:

| Instruction Out | Decoder In |
| --------------- | ---------- |
| IO0             | A7         |
| IO1             | A6         |
| IO2             | A5         |
| IO3             | A4         |
| IO4             | A3         |
| IO5             | A2         |
| IO6             | A1         |
| IO7             | A0         |
