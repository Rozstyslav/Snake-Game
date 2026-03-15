<h1 align="center">🐍 Snake Game</h1>
<h3 align="center">STM32 + Python + Qt/QML</h3>

<p align="center">
  A hybrid embedded + desktop Snake game project
</p>

<p align="center">
  <img src="https://img.shields.io/badge/STM32-MCU-orange?style=for-the-badge" alt="STM32">
  <img src="https://img.shields.io/badge/Python-3.x-blue?style=for-the-badge" alt="Python">
  <img src="https://img.shields.io/badge/Qt-QML-green?style=for-the-badge" alt="Qt">
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge" alt="Platform">
</p>

---

## 📖 Overview

This project combines **embedded programming** and **desktop application development** in a single Snake Game system.

The **game logic** runs on an **STM32 microcontroller** written in **C**, while the **graphical user interface** is implemented in **Python using Qt/QML**.

The desktop application communicates with the microcontroller through a **custom serial protocol**, sending control commands and receiving game state updates in real time.

### The project demonstrates:
- embedded systems integration
- desktop UI development with Qt/QML
- custom serial communication
- real-time interaction between hardware and software

---

## ✨ Features

<ul>
  <li>🐍 Classic Snake gameplay</li>
  <li>⚡ Real-time communication between PC and STM32</li>
  <li>🖥 Desktop interface built with <b>Qt / QML (PySide6)</b></li>
  <li>🔌 Custom binary serial protocol between PC and MCU</li>
  <li>🎮 Keyboard control from PC</li>
  <li>🧠 Game logic handled by STM32</li>
  <li>📊 Score and snake length tracking</li>
  <li>⚙ Configurable game parameters:
    <ul>
      <li>Speed</li>
      <li>Walls</li>
      <li>Movement restrictions</li>
    </ul>
  </li>
  <li>🌙 Support for light / dark themes</li>
  <li>🧾 Debug logging system</li>
</ul>

---

## 🏗 Architecture

<p>The project consists of three main components:</p>

```text
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
