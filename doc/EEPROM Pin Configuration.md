## Pins

| EEPROM | Shift Register | Flasher | 7-Seg | PC  | Inst | Num Reg | Microcode |
| ------ | -------------- | ------- | ----- | --- | ---- | ------- | --------- |
| IO 0   |                | A5      | 6     |     |      |         | 0         |
| IO 1   |                | A4      | 5     |     |      |         | 1         |
| IO 2   |                | A3      | 4     |     |      |         | 2         |
| IO 3   |                | D12     |       |     |      |         | 7         |
| IO 4   |                | D11     | 3     |     |      |         | 6         |
| IO 5   |                | D10     | 2     |     |      |         | 5         |
| IO 6   |                | D9      | 1     |     |      |         | 4         |
| IO 7   |                | D8      | 0     |     |      |         | 3         |
| ~OE    |                | D7      |       |     |      |         |           |
| ~WE    |                | D6      |       |     |      |         |           |
| A0     | QH (High)      |         |       | 7   | 7    | 0       |           |
| A1     | QG             |         |       | 6   | 6    | 1       |           |
| A2     | QF             |         |       | 5   | 5    | 2       |           |
| A3     | QE             |         |       | 4   | 4    | 3       |           |
| A4     | QD             |         |       | 3   | 3    | 4       |           |
| A5     | QC             |         |       | 2   | 2    | 5       |           |
| A6     | QB             |         |       | 1   | 1    | 6       |           |
| A7     | QA (Low)       |         |       | 0   | 0    | 7       |           |
| A8     |                | D5      |       |     |      |         |           |
|        | SRCLK          | D4      |       |     |      |         |           |
|        | RCLK           | D3      |       |     |      |         |           |
|        | SER            | D2      |       |     |      |         |           |

## 7-Segment Display
```
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

0: 01111110
1: 00011000
2: 10110110
3: 10111100
4: 11011000
5: 11101100
6: 11101110
7: 00111000
8: 11111110
9: 11111100
A: 11111010
B: 11001110
C: 01100110
D: 10011110
E: 11100110
F: 11100010
```

## Control Signals

```
EEPROM 1 (0 - 7)
~XO: X Output Enable
~SO (~DO): Sum/Diff Output Enable
~YO: Y Output Enable
~AO: A Output Enable
~LO: Left Shift Output Enable
~BO: Buttons Output Enable
~MO: Memory Output Enable
~GO: G Output Enable

EEPROM 2 (0 - 7)
~CO: C Output Enable
~JO: Jump Output Enable
~PO: PC Output Enable
~IO: Immediate Output Enable
XI: X Input Enable
YI: Y Input Enable
AI: A Input Enable
MI: Memory Input Enable

EEPROM 3 (0 - 7)
GI: G Input Enable
CI: C Input Enable
PI: PC Input Enable
NI: Number Input Enable
SB: Sub Enable
CN: Conditional Negate Enable
HT: Halt
~RS: Reset
```