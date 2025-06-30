# ZASM Code Style Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Project Structure](#project-structure)
3. [Naming Conventions](#naming-conventions)
4. [Documentation](#documentation)

## Introduction

This style guide defines the coding standards and best practices for the ZASM project. Adhering to these guidelines ensures code consistency, readability, and maintainability across the codebase. All contributors are expected to follow these conventions when writing or reviewing code.

## Project Structure

The project directory is organized as follows:

- `/src`: Contains the source code for the ZASM toolchain.
- `/nano`: Contains the Arduino Nano EEPROM flasher code.
- `/asm`: Contains assembly code for the 8-bit computer.
- `/out`: Build output directory (ignored by version control).
- `/doc`: Contains project documentation.
- `build script`: Located in the root folder of the project.

## Naming Conventions

Follow these naming conventions to ensure consistency and clarity throughout the ZASM codebase:

### Module and File Naming

- **Header files (`.h`) and source files (`.c`)**: Use lowercase with underscores, e.g., `module_name.h`, `module_name.c`.

### Type Definitions

- **Module Abbreviation**: Use 2-3 uppercase letters to represent the module (e.g., `DP` for Data Parser).
- **Structs/Unions**: `MOD_StructName_T` (UpperCamelCase, suffixed with `_T`).
- **Enums**: `MOD_EnumName_E` (UpperCamelCase, suffixed with `_E`).
- **Enum Members**: `MOD_ENUM_MEMBER_NAME` (all uppercase with underscores).
- **Typedefs for basic types/function pointers**: `MOD_CustomTypeName_T` (UpperCamelCase, suffixed with `_T`).

### Functions

- Prefix with the module abbreviation (uppercase), then an underscore, then a descriptive name in lowerCamelCase, e.g., `MOD_functionName()`.
- Exported functions must be declared in the corresponding `.h` file. Internal helper functions should be `static` and declared at the top of the `.c` file.

### Variables

- **Global variables (external linkage)**: Prefix with `g_`, then the module abbreviation, then an underscore, then a descriptive name in lowerCamelCase, e.g., `g_MOD_variableName`.
- **Static global variables (internal linkage)**: Prefix with `s_`, then the module abbreviation, then an underscore, then a descriptive name in lowerCamelCase, e.g., `s_MOD_variableName`.
- **Local variables**: Use lowerCamelCase, e.g., `localVariable`.

### Macros and Constants

- **Macros**: All uppercase with underscores, prefixed by the module abbreviation, e.g., `MOD_MACRO_NAME`.
- **Constants**:
  - Internal: `static const` with the `s_MOD_variableName` pattern, e.g., `static const int s_MOD_MaxAttempts = 5;`
  - Exported: `extern const` with the `g_MOD_variableName` pattern, e.g., `extern const int g_MOD_TimeoutSeconds;`

## Documentation

- **Doxygen** is the documentation format to be used.
- Function docstring (Doxygen comments) can only be written for the function declarations, not the function implementations.
- Functions should have inline comments within their bodies if there is something worth documenting about the implementation.
- All symbols should be documented, including global variables, macros, and types.
