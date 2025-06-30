# ZASM 8-bit Computer: Hardware Architecture

## Table of Contents

1. [Overview](#1-overview)
2. [I/O](#2-io)
3. [Internal Structure](#3-internal-structure)

## 1. Overview

This file documents the hardware architecture of the computer.

## 2. I/O

```text
 __________________ 
|                  |
| 8-bit DIP Switch |
|__ _ _ _ _ _ _ _ _|
|__*_*_*_*_*_*_*_*_|
   | | | | | | | |
   *-*-*-*-*-*-*-*
   v
   |
   8
   |
 __v_______________ 
|__*_______________|
|                  |
|  8-bit Computer  |
|__ _______________|
|__*_______________|
   v
   |
   8
   |
 __v_____________________ 
|__*_____________________|
|     Number Display     |
|    ____        ____    |
|   <____>      <____>   |
|  ^      ^    ^      ^  |
| | |    | |  | |    | | |
|  v ____ v    v ____ v  |
|  ^ ____ ^    ^ ____ ^  |
| | |    | |  | |    | | |
|  v ____ v    v ____ v  |
|   <____>      <____>   |
|________________________|
```

## 3. Internal Structure

```text
 ____________ 
|  |       | |
>OE| X Reg |O>
>LE|       |I<
|__|_______|_|
|_____D______|
      v
      |
 _____v______
|_____A______|
|  |       | |
>SB|  ALU  |O>
>OE|       | |
|__|_______|_|
|_____B______|
      ^
      |
 _____^______ 
|_____D______|
|  |       | |
>OE| Y Reg |O>
>LE|       |I<
|__|_______|_|


 ____________ 
|  |       | |
>OE|       |O>
>SE| A Reg |S>
>LE|       |I<
|__|_______|_|
|_____D______|
      v
      |
 _____v______
|_____A______|
|  |       | |
>OE|  RAM  |O>
>LE|       |I<
|__|_______|_|


 ____________ 
|  |       | |
>OE| G Reg |O>
>LE|       |I<
|__|_______|_|


 ____________ 
|  |       | |
>OE|       |O>
>JE| C Reg |J>
>LE|       |I<
|__|_______|_|
|_____J______|
      v
      |
 _____v______
|_____J______|
|  |       | |
>OE|       |O>
>LE|  PC   |I<
>CN|       | |
|__|_______|_|
|_____D______|
      v
      |
 _____v______
|_____A______|
|  |       | |
>OE|  ROM  |O>
|__|_______|_|
|_____D______|
      v
      |
 _____v______
|_____A______|
|            |
|    Inst    |
|____________|


 ____________ 
|  |       | |
>LE| N Reg |I<
|__|_______|_|


 ____________ 
|  |       | |
>OE|  DIP  |O>
|__|_______|_|
```

Note:

- Each box is a component.
- The left side is the control signals.
- The right side is the Bus in/out.
- The top and bottom are connections with other parts.
- The middle part is the name of the component.
- See [EEPROM Pin Configuration](EEPROM%20Pin%20Configuration.md) for explanations for each control signal.
