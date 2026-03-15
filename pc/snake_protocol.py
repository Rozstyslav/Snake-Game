import serial
import threading
import time
from datetime import datetime
from collections import deque
import serial.tools.list_ports
import os

HEADER = 0xAA
FRAME_SIZE = 8
DEBUG_FILE = os.path.join(os.path.dirname(__file__), "snake_debug_log.txt")

# Commands PC -> STM
CMD_SET_STATE = 0x01
CMD_MOVE = 0x02
CMD_CONFIG = 0x03
CMD_DEBUG = 0x10

# State
CMD_MENU = 0x00
CMD_START = 0x01
CMD_PAUSE = 0x02
CMD_GAME_OVER = 0x03
CMD_SLEEP = 0x04
CMD_EXIT = 0x05

# Move
CMD_MOVE_UP = 0x00
CMD_MOVE_DOWN = 0x01
CMD_MOVE_LEFT = 0x02
CMD_MOVE_RIGHT = 0x03

# Configuration
CMD_SPEED = 0x00
CMD_WALL = 0x01
CMD_RESTRICTION = 0x02

# Speed
CMD_SPEED_1 = 0x00
CMD_SPEED_2 = 0x01
CMD_SPEED_3 = 0x02

# Wall
CMD_SET_LEV_0 = 0x00
CMD_SET_LEV_1 = 0x01
CMD_SET_LEV_2 = 0x02
CMD_SET_LEV_3 = 0x03
CMD_SET_LEV_4 = 0x04
CMD_SET_LEV_5 = 0x05

# Restriction on/off
CMD_RESTRICTION_OFF = 0x00
CMD_RESTRICTION_ON = 0x01

# Debug Modes
CMD_DEBUG_OFF = 0x00
CMD_DEBUG_RANDOM_PACKETS = 0x01
CMD_DEBUG_SILENT_FLAG = 0x02
CMD_DEBUG_SCENARIO_1 = 0x03
CMD_DEBUG_LOOPBACK = 0x04
CMD_DEBUG_ERROR_INJECTION = 0x05
CMD_DEBUG_STEP_MODE = 0x06

# Commands STM -> PC
CMD_GAME_STATE = 0x81
CMD_SNAKE_DELTA = 0x82
CMD_SCORE = 0x83

# Delta_Flag
CMD_DELTA_MOVE = 0x00
CMD_DELTA_WALL = 0x01

ACK = 0x06
NACK = 0x15


class SnakeProtocol:
    @staticmethod
    def auto_detect_port() -> str | None:
        ports = serial.tools.list_ports.comports()

        for p in ports:
            desc = (p.description or "")
            print(f"Found: {p.device} - {desc}")

            up = desc.upper()
            if ("STM" in up) or ("STLINK" in up) or ("ST" in up):
                print(f"[AUTO] Using detected STM port: {p.device}")
                return p.device

        if ports:
            print("[AUTO] STM not clearly identified. Using first available port.")
            return ports[0].device

        return None

    def __init__(self, port=None, baudrate=9600, enable_keyboard=False):
        if port is None:
            port = self.auto_detect_port()

        if port is None:
            raise RuntimeError("No COM ports found")

        self.port = port
        self.baudrate = baudrate
        self.enable_keyboard = enable_keyboard

        # ---- EVENTS QUEUE ----
        self._events = deque()
        self._ev_lock = threading.Lock()
        # ----------------------

        # anti-spam RX error
        self._rx_err_last_ts = 0.0
        self._rx_err_interval = 1.5

        # state flags
        self.running = True
        self.game_active = False
        self.is_paused = False
        self.in_config = False
        self.in_menu = True
        self.is_game_over = False
        self.is_debug = False

        self.head_x, self.head_y = 25, 25
        self.x2, self.y2 = 0, 0
        self.snake_length = 0
        self.current_score = 0

        try:
            self.serial_pc_connection = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1,
                write_timeout=0.2
            )
            self.serial_pc_connection.reset_input_buffer()
            self.serial_pc_connection.reset_output_buffer()
        except serial.SerialException as e:
            raise RuntimeError(f"Could not open port {self.port}: {e}") from e

        try:
            with open(DEBUG_FILE, "w", encoding="utf-8") as f:
                f.write(f"--- Session started at {datetime.now()} ---\n")
        except Exception as e:
            print(f"Could not initialize log file: {e}")

        self.rx_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.rx_thread.start()

        if self.enable_keyboard:
            import keyboard
            self._keyboard = keyboard
            self.key_thread = threading.Thread(target=self._keyboard_loop, daemon=True)
            self.key_thread.start()

    # ================= LOGGING =================
    def log(self, message: str):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        formatted_msg = f"[{timestamp}] {message}"

        now = time.time()
        if not hasattr(self, "_last_log_line"):
            self._last_log_line = ""
            self._last_log_ts = 0.0

        if formatted_msg == self._last_log_line and (now - self._last_log_ts) < 0.25:
            return

        self._last_log_line = formatted_msg
        self._last_log_ts = now

        try:
            with open(DEBUG_FILE, "a", encoding="utf-8") as f:
                f.write(formatted_msg + "\n")
        except Exception:
            pass

    def _rx_log_limited(self, msg: str):
        now = time.time()
        if (now - self._rx_err_last_ts) >= self._rx_err_interval:
            self._rx_err_last_ts = now
            self.log(msg)

    # ============ EVENTS API ============
    def _push_event(self, evt):
        with self._ev_lock:
            self._events.append(evt)

    def pop_events(self, max_n=200):
        out = []
        with self._ev_lock:
            while self._events and len(out) < max_n:
                out.append(self._events.popleft())
        return out

    def clear_events(self):
        with self._ev_lock:
            self._events.clear()
    # ====================================

    # ================= Utility =================
    def calculate_crc(self, frame7bytes):
        crc = 0
        for b in frame7bytes:
            crc ^= b
        return crc

    def build_frame(self, cmd, data=None):
        if data is None:
            data = []
        frame = [HEADER, cmd]
        for i in range(5):
            frame.append(data[i] if i < len(data) else 0)
        frame.append(self.calculate_crc(frame))
        return bytes(frame)

    def send_frame(self, cmd, data=None):
        frame = self.build_frame(cmd, data)
        try:
            if not self.serial_pc_connection or not self.serial_pc_connection.is_open:
                self._rx_log_limited("[PC] TX Error: port not open")
                return
            self.serial_pc_connection.write(frame)
            self.log(f"[PC -> STM] TX CMD {hex(cmd)} : {frame.hex()}")
        except serial.SerialTimeoutException:
            self.log("[PC] TX Error: SerialTimeoutException (write timeout)")
        except (serial.SerialException, OSError) as e:
            self._rx_log_limited(f"[PC] TX Error: {e}")
        except Exception as e:
            self.log(f"[PC] TX Error: {e}")

    # ================= Debug Logic =================
    def set_debug_mode(self, mode):
        self.is_debug = True

        if mode == CMD_DEBUG_OFF:
            print("DEBUG: NORMAL MODE RESTORED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_OFF])
            self.is_debug = False
        elif mode == CMD_DEBUG_RANDOM_PACKETS:
            print("DEBUG: CHAOS MODE (Random Packets) ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_RANDOM_PACKETS])
        elif mode == CMD_DEBUG_SILENT_FLAG:
            print("DEBUG: SILENT MODE (LED only) ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_SILENT_FLAG])
        elif mode == CMD_DEBUG_SCENARIO_1:
            print("DEBUG: SCENARIO 1 (Predefined Move) ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_SCENARIO_1])
        elif mode == CMD_DEBUG_LOOPBACK:
            print("DEBUG: LOOPBACK MODE ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_LOOPBACK])
        elif mode == CMD_DEBUG_ERROR_INJECTION:
            print("DEBUG: ERROR INJECTION (Bad CRC) ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_ERROR_INJECTION])
        elif mode == CMD_DEBUG_STEP_MODE:
            print("DEBUG: STEP MODE ACTIVATED")
            self.send_frame(CMD_DEBUG, [CMD_DEBUG_STEP_MODE])

    # ================= Game Logic =================
    def start_game(self, clear_queue: bool = True):
        self.log(f"[PC] START GAME REQUEST (clear_queue={clear_queue})")
        if clear_queue:
            self.clear_events()

        self.send_frame(CMD_SET_STATE, [CMD_START])
        self.game_active, self.is_paused, self.is_game_over, self.in_config, self.in_menu = True, False, False, False, False

    def game_pause(self):
        if self.game_active and not self.is_paused:
            self.log("[PC] PAUSE REQUEST")
            self.send_frame(CMD_SET_STATE, [CMD_PAUSE])
            self.is_paused, self.game_active = True, False

    def resume_game(self):
        if self.is_paused:
            self.log("[PC] RESUME REQUEST")
            self.send_frame(CMD_SET_STATE, [CMD_START])
            self.is_paused, self.game_active = False, True

    def config(self):
        self.log("[PC] ENTER CONFIG")
        self.in_config = True
        self.game_active = False
        self.is_paused = False
        self.in_menu = False

    def back_to_menu(self):
        self.log("[PC] BACK TO MENU")
        self.clear_events()
        self.send_frame(CMD_SET_STATE, [CMD_MENU])
        self.in_menu, self.game_active, self.is_paused, self.in_config, self.is_game_over = True, False, False, False, False

    def exit_game(self):
        self.log("[PC] EXIT PROGRAM")
        self.send_frame(CMD_SET_STATE, [CMD_EXIT])
        self.running = False

    def move(self, direction):
        if self.game_active:
            self.send_frame(CMD_MOVE, [int(direction)])

    def set_speed(self, speed_level):
        self.log(f"[PC] SET SPEED -> {speed_level} (in_config={self.in_config})")
        self.send_frame(CMD_CONFIG, [CMD_SPEED, int(speed_level)])

    def restriction_on(self):
        self.log(f"[PC] SET RESTRICTION -> ON (in_config={self.in_config})")
        self.send_frame(CMD_CONFIG, [CMD_RESTRICTION, CMD_RESTRICTION_ON])

    def restriction_off(self):
        self.log(f"[PC] SET RESTRICTION -> OFF (in_config={self.in_config})")
        self.send_frame(CMD_CONFIG, [CMD_RESTRICTION, CMD_RESTRICTION_OFF])

    def set_wall(self, wall_level):
        self.log(f"[PC] SET WALL LEVEL -> {wall_level} (in_config={self.in_config})")
        self.send_frame(CMD_CONFIG, [CMD_WALL, int(wall_level)])

    # ================= Optional keyboard loop =================
    def _keyboard_loop(self):
        while self.running:
            try:
                if self.game_active:
                    if self._keyboard.is_pressed("w"):
                        self.move(CMD_MOVE_UP); time.sleep(0.15)
                    elif self._keyboard.is_pressed("s"):
                        self.move(CMD_MOVE_DOWN); time.sleep(0.15)
                    elif self._keyboard.is_pressed("a"):
                        self.move(CMD_MOVE_LEFT); time.sleep(0.15)
                    elif self._keyboard.is_pressed("d"):
                        self.move(CMD_MOVE_RIGHT); time.sleep(0.15)
            except:
                pass
            time.sleep(0.01)

    # ================= RX =================
    def _receive_loop(self):
        buffer = bytearray()
        while self.running:
            try:
                if not self.serial_pc_connection or not self.serial_pc_connection.is_open:
                    time.sleep(0.05)
                    continue

                waiting = self.serial_pc_connection.in_waiting
                if waiting:
                    data = self.serial_pc_connection.read(waiting)
                    if data:
                        buffer.extend(data)

                    while len(buffer) > 0:
                        if buffer[0] in (ACK, NACK):
                            byte = buffer.pop(0)
                            res_name = "ACK" if byte == ACK else "NACK"
                            self.log(f"[STM -> PC] {res_name} : {byte:02x}")
                            continue

                        if len(buffer) < FRAME_SIZE:
                            break

                        if buffer[0] != HEADER:
                            buffer.pop(0)
                            continue

                        frame = buffer[:FRAME_SIZE]
                        buffer = buffer[FRAME_SIZE:]
                        self._parse_frame(frame)

            except (serial.SerialException, OSError) as e:
                if self.running:
                    self._rx_log_limited(f"[RX Error] {e}")
                time.sleep(0.05)
            except Exception as e:
                if self.running:
                    self._rx_log_limited(f"[RX Error] {e}")
                time.sleep(0.01)

            time.sleep(0.005)

    def _parse_frame(self, frame: bytes):
        crc = frame[7]
        calc_crc = self.calculate_crc(frame[:7])

        cmd = frame[1]
        data = frame[2:7]

        if crc != calc_crc:
            try:
                if self.serial_pc_connection and self.serial_pc_connection.is_open:
                    self.serial_pc_connection.write(bytes([NACK]))
            except:
                pass
            self.log(f"[STM -> PC] CRC ERROR -> NACK | Frame: {frame.hex()}")
            return

        try:
            if self.serial_pc_connection and self.serial_pc_connection.is_open:
                self.serial_pc_connection.write(bytes([ACK]))
        except:
            pass

        self.log(f"[STM -> PC] {hex(cmd)} : {frame.hex()}")

        if cmd == CMD_GAME_STATE:
            st = int(data[0])
            if st == CMD_GAME_OVER:
                self.is_game_over = True
                self.game_active = False
                self.is_paused = False
                self._push_event(("state", "game_over"))
                self.log(f"[STM -> PC] GAME OVER {hex(cmd)} : {frame.hex()}")
            else:
                self._push_event(("state", st))

        elif cmd == CMD_SNAKE_DELTA:
            x1 = int(data[0])
            y1 = int(data[1])
            x2 = int(data[2])
            y2 = int(data[3])
            delta_flag = int(data[4])

            if delta_flag == CMD_DELTA_MOVE:
                self.head_x, self.head_y = x1, y1
                self.x2, self.y2 = x2, y2
                self.log(f"[STM] DELTA MOVE: ({x1},{y1}) ({x2},{y2}) flag({delta_flag})")
            elif delta_flag == CMD_DELTA_WALL:
                self.log(f"[STM] DELTA WALL: ({x1},{y1}) ({x2},{y2}) flag({delta_flag})")

            self._push_event(("delta", x1, y1, x2, y2, delta_flag))

        elif cmd == CMD_SCORE:
            ln = int(data[0])
            sc = int(data[1])
            self.snake_length = ln
            self.current_score = sc
            self._push_event(("score", ln, sc))
            self.log(f"[STM] SCORE: Length {ln}, Points {sc}")

    def close(self):
        self.running = False
        try:
            if self.serial_pc_connection and self.serial_pc_connection.is_open:
                self.serial_pc_connection.close()
        except:
            pass
        try:
            if hasattr(self, "rx_thread") and self.rx_thread and self.rx_thread.is_alive():
                self.rx_thread.join(timeout=0.8)
        except:
            pass