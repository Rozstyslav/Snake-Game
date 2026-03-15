Technical documentation describing the agreed UART communication protocol between STM32 and PC (Python).
Overview
The protocol provides real-time two-way data exchange between PC and STM32 via UART.
Communication is implemented between:
- STM32 (game logic side)
- PC (Python, pyserial) (control & visualization side)
This protocol is used to control and monitor a Snake game.

Architecture
PC (Python)  <-- UART -->  STM32

PC:
- Sends control commands
- Receives game state updates
- Validates checksum

STM32:
- Handles commands
- Updates game logic
- Sends delta updates
- Sends score and state

Frame Structure
Each protocol command consists of 8 bytes.
| Byte | Description    |
| ---- | -------------- |
| 0    | Header (0xAA)  |
| 1    | Command code   |
| 2–6  | Data (5 bytes) |
| 7    | Checksum       |

Command Ranges :
PC → STM: 0x01 – 0x1F
STM → PC: 0x81 – 0x9F
