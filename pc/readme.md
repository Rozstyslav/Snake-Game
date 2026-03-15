Snake PC Client (UART, STM32)
Python client for controlling and monitoring a Snake game running on STM32 via UART.
This program:
- Sends control commands (START, MOVE, STOP)
- Receives game updates (DELTA, SCORE, FOOD)
- Validates CRC
- Sends ACK / NACK
- Implements keyboard control
- Performs basic PC-side logical validation

Communication Overview :
- Protocol frame size: 8 bytes
- Header: 0xAA
- CRC: XOR of first 7 bytes
- ACK: 0x06
- NACK: 0x15
- Baudrate: 9600

Supported Commands :
PC → STM32 :
| Command | Code | Description                 |
| ------- | ---- | --------------------------- |
| START   | 0x01 | Start game                  |
| MOVE    | 0x02 | Move snake                  |
| CONFIG  | 0x03 | Configuration (placeholder) |
| STOP    | 0x0F | Stop game                   |

STM32 → PC
| Command     | Code | Description                |
| ----------- | ---- | -------------------------- |
| GAME_STATE  | 0x81 | Game state update          |
| SNAKE_DELTA | 0x82 | Snake and food head update |
| SCORE       | 0x83 | Score update               |

Controls
| Key | Action               |
| --- | -------------------- |
| T   | Start game           |
| O   | Config (placeholder) |
| Q   | Back to menu         |  
| W   | Move Up              |
| S   | Move Down            |
| A   | Move Left            |
| D   | Move Right           |

Multithreading
The client uses two daemon threads:
- RX thread → UART receive loop
- Keyboard thread → real-time input handling
This allows:
- Non-blocking UART
- Real-time control
- Continuous frame parsing

Requirements
- Python 3.10+
- pyserial
- keyboard

How to Run
- Connect STM32 via UART
