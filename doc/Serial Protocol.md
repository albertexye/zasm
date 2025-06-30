# ZASM 8-bit Computer: Serial Protocol

## Table of Contents

1. [Introduction](#1-introduction)
2. [Physical Layer](#2-physical-layer)
3. [Packet Structure](#3-packet-structure)
4. [Operation Codes](#4-operation-codes)
5. [CRC16 Calculation](#5-crc16-calculation)
6. [Protocol Flow](#6-protocol-flow)
7. [Error Handling](#7-error-handling)
8. [Host-Device Command Reference](#8-host-device-command-reference)

## 1. Introduction

This custom serial protocol is used to communicate with the EEPROM flasher. The flasher is an Arduino Nano connected to two 74HC595 shift registers. The protocol enables the host to program the EEPROM by sending data and control commands over a serial connection.

## 2. Physical Layer

- **Baud Rate:** 115200
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None

The Arduino Nano is connected to the host via USB serial. The Nano controls EEPROM address and data lines through two 74HC595 shift registers. See [EEPROM Pin Configuration](EEPROM%20Pin%20Configuration.md) for hardware details.

## 3. Packet Structure

All packets begin with a fixed header and are followed by an operation code, optional data, and a CRC16 checksum.

### General Packet Format

| Offset | Field         | Size (bytes) | Description                        |
|--------|--------------|--------------|------------------------------------|
| 0      | Magic        | 1            | Always `0xAA`                      |
| 1      | OpCode       | 1            | Operation code (see below)         |
| 2..n   | Data         | 0 or 256     | Data payload (for write commands)  |
| n+1    | CRC16 (MSB)  | 1            | CRC16 high byte                    |
| n+2    | CRC16 (LSB)  | 1            | CRC16 low byte                     |

- For commands without data (e.g., PING), the packet is 4 bytes: Magic, OpCode, CRC16(MSB), CRC16(LSB).
- For write commands, the packet is 260 bytes: Magic, OpCode, 256 bytes data, CRC16(MSB), CRC16(LSB).

## 4. Operation Codes

| Code | Name        | Value | Description                        |
|------|-------------|-------|------------------------------------|
| 0    | OP_PING     | 0x00  | Ping the device                    |
| 1    | OP_SEND_LOW | 0x01  | Write 256 bytes to low EEPROM page |
| 2    | OP_SEND_HIGH| 0x02  | Write 256 bytes to high EEPROM page|
| 3    | OP_ACK      | 0x03  | Acknowledge (sent by device only)  |

## 5. CRC16 Calculation

- **Polynomial:** 0x8005
- **Initial Value:** 0
- **Algorithm:** Bitwise, as implemented in both host and device code.

### CRC Calculation Steps

1. For each byte in the packet (excluding the CRC bytes), update the CRC using the `crc16Update` function.
2. After all bytes, finalize the CRC with `crc16Finalize`.
3. Append the resulting 2-byte CRC (MSB first, then LSB) to the packet.

**On the device, the CRC is checked for all data packets. If the CRC does not match, the packet is dropped.**

## 6. Protocol Flow

### 6.1. Ping

- **Host → Device:** Send a 4-byte packet: `[0xAA, OP_PING, 0xA0, 0x7E]`
  - The last two bytes are a fixed CRC for the ping command.
- **Device → Host:** Responds with ACK packet: `[0xAA, OP_ACK, 0xA1, 0x3E]`

### 6.2. Write Data

- **Host → Device:** Send a 260-byte packet:
  - `[0xAA, OP_SEND_LOW or OP_SEND_HIGH, 256 bytes data, CRC16(MSB), CRC16(LSB)]`
- **Device:** If CRC is valid, writes data to EEPROM (low or high page), then responds with ACK.

### 6.3. Acknowledge

- **Device → Host:** After successful operation, device sends `[0xAA, OP_ACK, 0xA1, 0x3E]`.

## 7. Error Handling

- **Invalid Magic Byte:** Device ignores the packet and blinks the onboard LED 4 times.
- **Invalid OpCode:** Device ignores the packet and blinks the onboard LED 4 times.
- **CRC Mismatch:** Device ignores the packet and blinks the onboard LED 4 times.
- **Timeout:** If the device does not receive expected bytes within 500ms, it aborts the operation.

The host can use the error codes and messages defined in `zasmf.h` and `zasmf.c` to interpret protocol and system errors.

## 8. Host-Device Command Reference

### 8.1. PING

- **Purpose:** Check if the device is present and responsive.
- **Host Packet:** `[0xAA, 0x00, 0xA0, 0x7E]`
- **Device Response:** `[0xAA, 0x03, 0xA1, 0x3E]`

### 8.2. WRITE LOW PAGE

- **Purpose:** Write 256 bytes to the low page of EEPROM.
- **Host Packet:** `[0xAA, 0x01, <256 bytes data>, CRC16(MSB), CRC16(LSB)]`
- **Device Response:** `[0xAA, 0x03, 0xA1, 0x3E]` (on success)

### 8.3. WRITE HIGH PAGE

- **Purpose:** Write 256 bytes to the high page of EEPROM.
- **Host Packet:** `[0xAA, 0x02, <256 bytes data>, CRC16(MSB), CRC16(LSB)]`
- **Device Response:** `[0xAA, 0x03, 0xA1, 0x3E]` (on success)

### 8.4. ACKNOWLEDGE

- **Purpose:** Device signals successful operation.
- **Packet:** `[0xAA, 0x03, 0xA1, 0x3E]`

**Note:**  

- The host must wait for an ACK after each command before sending the next command.
- The device flushes its serial buffer after each command to ensure clean state.

**See also:**  

- [EEPROM Pin Configuration](EEPROM%20Pin%20Configuration.md) for hardware wiring.
- [nano.ino](../nano/nano.ino) for device firmware.
- [zasmf.h](../src/zasmf.h) and [zasmf.c](../src/zasmf.c) for host-side implementation.
