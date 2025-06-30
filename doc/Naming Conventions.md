# C Project Naming Convention

This document outlines the naming conventions to be followed for the C project. Adhering to these guidelines ensures consistency, readability, and maintainability across all modules.

-----

## 1\. Module Naming

* **Header Files (`.h`):** Use **lowercase with underscores**.

  * `module_name.h`
  * *Examples:* `data_parser.h`, `network_manager.h`

* **Source Files (`.c`):** Match the corresponding header file name.

  * `module_name.c`
  * *Examples:* `data_parser.c`, `network_manager.c`

-----

## 2\. Type Definitions

All type definitions (structs, enums, unions, and `typedef`s for basic types/function pointers) will include a module abbreviation prefix.

* **Module Abbreviation:** Use **2-3 uppercase letters** representing the module (e.g., `DP` for Data Parser, `NM` for Network Manager).

* **Structs and Unions:** Prefix with the module abbreviation, followed by an underscore, then a descriptive name using **UpperCamelCase**, and finally suffix with `_T`.

  * `MOD_StructName_T`
  * *Examples:* `DP_ParsedData_T`, `NM_ConnectionInfo_T`

* **Enums:** Prefix with the module abbreviation, followed by an underscore, then a descriptive name using **UpperCamelCase**, and finally suffix with `_E`.

  * `MOD_EnumName_E`
  * *Examples:*

        ```c
        typedef enum {
            DP_PARSE_STATUS_SUCCESS,
            DP_PARSE_STATUS_ERROR_INVALID_FORMAT,
            DP_PARSE_STATUS_ERROR_READ_FAIL
        } DP_ParseStatus_E;
        ```

* **Enumeration Members:** Prefix with the module abbreviation, followed by the enum name (all **uppercase with underscores**), then the member name (all **uppercase with underscores**).

  * `MOD_ENUMNAME_MEMBER_NAME`
  * *Examples:* `DP_PARSE_STATUS_SUCCESS`, `NM_CONNECTION_STATE_CONNECTED`

* **Typedefs for Basic Types (e.g., Function Pointers):** Prefix with the module abbreviation, followed by an underscore, then a descriptive name using **UpperCamelCase**, and finally suffix with `_T`.

  * `MOD_CustomTypeName_T`
  * *Example:* `NM_PacketHandler_T` (for a function pointer type)

-----

## 3\. Functions

* Prefix with the module abbreviation (uppercase) followed by an underscore, then a descriptive name using **lowerCamelCase**.

  * `MOD_functionName()`
  * *Examples:* `DP_parseData()`, `NM_connectToServer()`

* Every functions must be decalred first. Exported functions must be declared in the corresponding .h file, whereas internal helper functions must be static and declared at the beginning of the .c file. Exported functions don't need to be declared in the .c file.

-----

## 4\. Variables

Variable naming depends on their scope and linkage.

* **Global Variables (External Linkage):** If absolutely necessary (declared in `.h`, defined in `.c`), prefix with `g_` (for global), then the module abbreviation (uppercase) followed by an underscore, and finally a descriptive name using **lowerCamelCase**.

  * `extern MOD_Type_T g_MOD_variableName;` (in `.h`)
  * `MOD_Type_T g_MOD_variableName;` (in `.c`)
  * *Example:* `extern DP_ParsedData_T g_DP_lastParsedData;`

* **Static Global Variables (Internal Linkage):** These are `static` variables within a `.c` file. Prefix with `s_` (for static), then the module abbreviation (uppercase) followed by an underscore, and finally a descriptive name using **lowerCamelCase**.

  * `static MOD_Type_T s_MOD_variableName;`
  * *Example:* `static NM_ConnectionInfo_T s_NM_currentConnection;`

* **Local Variables:** Use **lowerCamelCase**.

  * `localVariable`
  * *Examples:* `int count;`, `char* buffer;`

-----

## 5\. Macros and Constants

* **Macros (Preprocessor Definitions):** Use all **uppercase with underscores**, prefixed by the module abbreviation.

  * `MOD_MACRO_NAME`
  * *Examples:* `DP_MAX_BUFFER_SIZE`, `NM_DEFAULT_PORT`

* **Constants (`const` keyword):**

  * **Internal Module Constants:** Use `static const` and follow the `s_MOD_variableName` pattern for static variables, but often in **UpperCamelCase** for the constant's specific name.
    * *Example:* `static const int s_DP_MaxAttempts = 5;`
  * **Exported Constants (if truly necessary):** Use `extern const` and follow the `g_MOD_variableName` pattern for global variables, often in **UpperCamelCase**.
    * *Example:* `extern const int g_NM_TimeoutSeconds;`

-----
