# 🐍 Snake Game (STM32 + Python + Qt)

A hybrid embedded + desktop Snake game project where the game logic runs on an STM32 microcontroller, while the graphical interface is implemented in Python using Qt/QML.

The PC application communicates with the microcontroller via serial protocol, sending control commands and receiving game state updates in real time.

This project demonstrates embedded systems integration with desktop UI, custom communication protocols, and real-time interaction between hardware and software.

📌 Features

🐍 Classic Snake gameplay

⚡ Real-time communication between PC and STM32

🖥 Desktop interface built with Qt / QML (PySide6)

🔌 Custom binary serial protocol between PC and MCU

🎮 Keyboard control from PC

🧠 Game logic handled by STM32

📊 Score and snake length tracking

⚙ Configurable game parameters:

Speed

Walls

Movement restrictions

🌙 Support for light / dark themes

🧾 Debug logging system

🏗 Architecture

The project consists of three main components:
+-------------------+
|   Qt / QML UI     |
|  (Python PySide6) |
+---------+---------+
          |
          | Serial protocol
          |
+---------v---------+
| Python Backend    |
| snake_protocol.py |
+---------+---------+
          |
          | UART
          |
+---------v---------+
|     STM32 MCU     |
|   Game Logic (C)  |
+-------------------+

Responsibilities
Component	Description
STM32 (C)	Runs the game engine, snake movement, collision detection
Python backend	Handles serial communication and protocol
Qt/QML UI	Displays the game and processes user input

🧰 Technologies Used
Embedded

C

STM32 microcontroller

UART communication

Desktop

Python 3

PySide6 (Qt for Python)

QML UI

Tools

Qt Creator

STM32 development environment

PyInstaller (for packaging)

⚙ Installation
1️⃣ Clone repository
git clone https://github.com/Rozstyslav/Snake-Game.git
cd Snake-Game
2️⃣ Install dependencies
pip install PySide6 pyserial
3️⃣ Run application
python main.py
