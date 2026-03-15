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

## Component Responsibilities

<table>
  <thead>
    <tr>
      <th>Component</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>STM32 (C)</b></td>
      <td>Runs the game engine, snake movement, and collision detection</td>
    </tr>
    <tr>
      <td><b>Python backend</b></td>
      <td>Handles serial communication and protocol processing</td>
    </tr>
    <tr>
      <td><b>Qt/QML UI</b></td>
      <td>Displays the game and processes user input</td>
    </tr>
  </tbody>
</table>

---

## 🧰 Technologies Used

<h3>Embedded</h3>
<ul>
  <li><b>C</b></li>
  <li>STM32 microcontroller</li>
  <li>UART communication</li>
</ul>

<h3>Desktop</h3>
<ul>
  <li><b>Python 3</b></li>
  <li>PySide6 (Qt for Python)</li>
  <li>Qt / QML UI</li>
</ul>

<h3>Tools</h3>
<ul>
  <li>Qt Creator</li>
  <li>STM32 development environment</li>
  <li>PyInstaller (for packaging)</li>
</ul>

---

## ⚙ Installation

<h3>1️⃣ Clone repository</h3>

<pre><code>git clone https://github.com/Rozstyslav/Snake-Game.git
cd Snake-Game</code></pre>

<h3>2️⃣ Install dependencies</h3>

<pre><code>pip install PySide6 pyserial</code></pre>

<h3>3️⃣ Run application</h3>

<pre><code>python main.py</code></pre>
