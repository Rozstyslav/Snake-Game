import serial
import threading
import time
import keyboard
import sys
from datetime import datetime
import constants as c
import serial.tools.list_ports

class SnakeProtocol:
    def auto_detect_port(self, baudrate=9600, timeout=0.5):
        ports = serial.tools.list_ports.comports()

        if not ports:
            return None

        print("[AUTO] Scanning COM ports for STM (waiting for ACK)...")

        for port in ports:
            try:
                print(f"[AUTO] Testing {port.device}...")

                test_serial = serial.Serial(port=port.device,
                                            baudrate=baudrate,
                                            timeout=timeout)

                test_serial.reset_input_buffer()
                test_serial.reset_output_buffer()

                frame = [c.HEADER, c.CMD_SET_STATE,
                         c.CMD_MENU, 0, 0, 0, 0]

                # CRC
                crc = 0
                for b in frame:
                    crc ^= b
                frame.append(crc)

                test_serial.write(bytes(frame))

                time.sleep(0.2)

                if test_serial.in_waiting:
                    response = test_serial.read(test_serial.in_waiting)

                    if c.ACK in response:
                        print(f"[AUTO] STM found on {port.device}")
                        test_serial.close()
                        return port.device

                test_serial.close()

            except Exception as e:
                print(f"[AUTO] Skipping {port.device}: {e}")

        print("[AUTO] STM not found (no ACK received).")
        return None

        return None

    def __init__(self, port=None, baudrate=9600):

        if port is None:
            port = self.auto_detect_port(baudrate)

        if port is None:
            print("[ERROR] No COM ports found.")
            sys.exit(1)
        try:
            self.serial_pc_connection = serial.Serial(port=port, baudrate=baudrate, timeout=0.1)
            self.serial_pc_connection.reset_input_buffer()
            self.serial_pc_connection.reset_output_buffer()
        except serial.SerialException as e:
            print(f"[ERROR] Could not open port {port}: {e}")
            sys.exit(1)

        try:
            with open(c.DEBUG_FILE, "w", encoding="utf-8") as f:
                f.write(f"--- Session started at {datetime.now()} ---\n")
        except Exception as e:
            print(f"Could not initialize log file: {e}")

        self.running = True
        self.game_active = False
        self.is_paused = False
        self.in_config = False
        self.in_menu = True
        self.is_game_over = False
        self.is_debug = False

        # self.head_x, self.head_y = 25, 25
        # self.food_x, self.food_y = 0, 0
        # self.snake_length = 0
        # self.current_score = 0

        self.display_main_menu()

        self.rx_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.rx_thread.start()
        self.key_thread = threading.Thread(target=self._keyboard_loop, daemon=True)
        self.key_thread.start()

    # ================= LOGGING SYSTEM =================
    def log(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        formatted_msg = f"[{timestamp}] {message}"

        if self.is_debug:
            print(formatted_msg)

        try:
            with open(c.DEBUG_FILE, "a", encoding="utf-8") as f:
                f.write(formatted_msg + "\n")
        except Exception as e:
            print(f"File log error: {e}")

        if not self.is_debug and "---" not in message:
            if any(x in message for x in ["GAME OVER", "SCORE", "START"]):
                print(message)

    # ================= Utility =================
    def display_main_menu(self):
        print("\n=== MAIN MENU ===")
        print("T - Start Game | O - Config | Q - Exit Program")
        print("Debug: 1-Normal | 2-Chaos | L-Silent | 3-Scenario | 4-Loopback | 5-Error Inj | 6-Step mode\n")

    def calculate_crc(self, frame7bytes):
        crc = 0
        for b in frame7bytes:
            crc ^= b
        return crc

    def build_frame(self, cmd, data=None):
        if data is None: data = []
        frame = [c.HEADER, cmd]
        for i in range(5):
            frame.append(data[i] if i < len(data) else 0)
        frame.append(self.calculate_crc(frame))
        return bytes(frame)

    def send_frame(self, cmd, data=None):
        frame = self.build_frame(cmd, data)
        self.serial_pc_connection.write(frame)
        self.log(f"[PC -> STM] TX CMD {hex(cmd)} : {frame.hex()}")

    # ================= Debug Logic =================
    def set_debug_mode(self, mode):
        self.is_debug = True

        if mode == c.CMD_DEBUG_OFF:
            print("DEBUG: NORMAL MODE RESTORED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_OFF])
            self.is_debug = False
        elif mode == c.CMD_DEBUG_RANDOM_PACKETS:
            print("DEBUG: CHAOS MODE (Random Packets) ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_RANDOM_PACKETS])
        elif mode == c.CMD_DEBUG_SILENT_FLAG:
            print("DEBUG: SILENT MODE (LED only) ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_SILENT_FLAG])
        elif mode == c.CMD_DEBUG_SCENARIO_1:
            print("DEBUG: SCENARIO 1 (Predefined Move) ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_SCENARIO_1])
        elif mode == c.CMD_DEBUG_LOOPBACK:
            print("DEBUG: LOOPBACK MODE ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_LOOPBACK])
        elif mode == c.CMD_DEBUG_ERROR_INJECTION:
            print("DEBUG: ERROR INJECTION (Bad CRC) ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_ERROR_INJECTION])
        elif mode == c.CMD_DEBUG_STEP_MODE:
            print("DEBUG: STEP MODE ACTIVATED")
            self.send_frame(c.CMD_DEBUG, [c.CMD_DEBUG_STEP_MODE])

    # ================= Game Logic =================
    def start_game(self):
        self.log("[PC] START GAME REQUEST")
        self.send_frame(c.CMD_SET_STATE, [c.CMD_START])
        self.game_active, self.is_paused, self.is_game_over, self.in_config, self.in_menu = True, False, False, False, False

    def game_pause(self):
        if self.game_active and not self.is_paused:
            self.log("[PC] PAUSE REQUEST")
            self.send_frame(c.CMD_SET_STATE, [c.CMD_PAUSE])
            self.is_paused, self.game_active = True, False

    def resume_game(self):
        if self.is_paused:
            self.log("[PC] RESUME REQUEST")
            self.send_frame(c.CMD_SET_STATE, [c.CMD_START])
            self.is_paused, self.game_active = False, True

    def handle_game_over_event(self):
        self.log("!!! [STM] NOTIFIED: GAME OVER !!!")
        self.is_game_over = True
        self.game_active = False
        self.is_paused = False
        self.in_config = False
        self.in_menu = False
        print("\n--- GAME OVER ---")
        print("Press 'Q' to return to Menu or T to restart game")

    def config(self):
        print("\n=== CONFIG MENU ===")
        print("1 - Speed Level 1")
        print("2 - Speed Level 2")
        print("3 - Speed Level 3")
        print("W - Wall Level 0 (No walls)")
        print("E - Wall Level 1")
        print("R - Wall Level 2")
        print("T - Wall Level 3")
        print("Y - Wall Level 4")
        print("U - Wall Level 5")
        print("I - Wall Level 6")
        print("Q - Back to Menu\n")

        self.log("[PC] ENTER CONFIG")
        self.in_config = True
        self.game_active = False
        self.is_paused = False
        self.in_menu = False

    def set_speed(self, speed_level):
        if self.in_config:
            self.log(f"[PC] SET SPEED -> {speed_level}")
            self.send_frame(c.CMD_CONFIG, [c.CMD_SPEED, speed_level])

    def restriction_on(self):
        if self.in_config:
            self.log("[PC] SET RESTRICTION -> ON")
            self.send_frame(c.CMD_CONFIG, [c.CMD_RESTRICTION, c.CMD_RESTRICTION_ON])

    def restriction_off(self):
        if self.in_config:
            self.log("[PC] SET RESTRICTION -> OFF")
            self.send_frame(c.CMD_CONFIG, [c.CMD_RESTRICTION, c.CMD_RESTRICTION_OFF])

    def set_wall(self, level):
        if self.in_config:
            self.log(f"[PC] SET WALL LEVEL -> {level}")
            self.send_frame(c.CMD_CONFIG, [c.CMD_WALL, level])

    def back_to_menu(self):
        self.log("[PC] BACK TO MENU")
        self.send_frame(c.CMD_SET_STATE, [c.CMD_MENU])
        self.in_menu, self.game_active, self.is_paused, self.in_config, self.is_game_over = True, False, False, False, False
        self.display_main_menu()

    def exit_game(self):
        self.log("[PC] EXIT PROGRAM")
        self.send_frame(c.CMD_SET_STATE, [c.CMD_EXIT])
        self.running = False

    def move(self, direction):
        if self.game_active:
            self.send_frame(c.CMD_MOVE, [direction])

    # ================= Loops =================
    def _keyboard_loop(self):
        while self.running:
            try:
                # Debug Keys
                if not self.in_config:
                    if keyboard.is_pressed("l"):
                        self.set_debug_mode(c.CMD_DEBUG_SILENT_FLAG)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("1"):
                        self.set_debug_mode(c.CMD_DEBUG_OFF)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("2"):
                        self.set_debug_mode(c.CMD_DEBUG_RANDOM_PACKETS)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("3"):
                        self.set_debug_mode(c.CMD_DEBUG_SCENARIO_1)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("4"):
                        self.set_debug_mode(c.CMD_DEBUG_LOOPBACK)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("5"):
                        self.set_debug_mode(c.CMD_DEBUG_ERROR_INJECTION)
                        time.sleep(0.3)
                    elif keyboard.is_pressed("6"):
                        self.set_debug_mode(c.CMD_DEBUG_STEP_MODE)
                        time.sleep(0.3)

                # Navigation Keys
                if self.in_menu :
                    if keyboard.is_pressed("t"):
                        self.start_game()
                        time.sleep(0.3)
                    elif keyboard.is_pressed("o"):
                        self.config()
                        time.sleep(0.3)
                    elif keyboard.is_pressed("q"):
                        self.exit_game()

                # Config Keys
                elif self.in_config:
                    if keyboard.is_pressed("q"):
                        self.back_to_menu()
                        time.sleep(0.3)

                    elif keyboard.is_pressed("1"):
                        self.set_speed(c.CMD_SPEED_1)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("2"):
                        self.set_speed(c.CMD_SPEED_2)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("3"):
                        self.set_speed(c.CMD_SPEED_3)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("w"):
                        self.set_wall(c.CMD_SET_LEV_0)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("e"):
                        self.set_wall(c.CMD_SET_LEV_1)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("r"):
                        self.set_wall(c.CMD_SET_LEV_2)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("t"):
                        self.set_wall(c.CMD_SET_LEV_3)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("y"):
                        self.set_wall(c.CMD_SET_LEV_4)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("u"):
                        self.set_wall(c.CMD_SET_LEV_5)
                        time.sleep(0.3)

                    elif keyboard.is_pressed("i"):
                        self.set_wall(c.CMD_SET_LEV_6)
                        time.sleep(0.3)

                elif self.game_active:
                    if keyboard.is_pressed("w"):
                        self.move(c.CMD_MOVE_UP)
                        time.sleep(0.15)
                    elif keyboard.is_pressed("s"):
                        self.move(c.CMD_MOVE_DOWN)
                        time.sleep(0.15)
                    elif keyboard.is_pressed("a"):
                        self.move(c.CMD_MOVE_LEFT)
                        time.sleep(0.15)
                    elif keyboard.is_pressed("d"):
                        self.move(c.CMD_MOVE_RIGHT)
                        time.sleep(0.15)
                    elif keyboard.is_pressed("p"):
                        self.game_pause()
                        time.sleep(0.3)
                    elif keyboard.is_pressed("q"):
                        self.back_to_menu()
                        time.sleep(0.3)


                elif self.is_paused:
                    if keyboard.is_pressed("p"):
                        self.resume_game()
                        time.sleep(0.3)
                    elif keyboard.is_pressed("q"):
                        self.back_to_menu()
                        time.sleep(0.3)

                elif self.is_game_over:
                    if keyboard.is_pressed("q"):
                        self.back_to_menu()
                        time.sleep(0.3)
                    elif keyboard.is_pressed("t"):
                        self.start_game()
                        time.sleep(0.3)

            except:
                pass
            time.sleep(0.01)

    def _receive_loop(self):
        buffer = bytearray()
        while self.running:
            try:
                if self.serial_pc_connection.in_waiting:
                    data = self.serial_pc_connection.read(self.serial_pc_connection.in_waiting)
                    buffer.extend(data)
                    while len(buffer) > 0:
                        if buffer[0] in (c.ACK, c.NACK):
                            byte = buffer.pop(0)
                            res_name = "ACK" if byte == c.ACK else "NACK"
                            self.log(f"[STM -> PC] {res_name} : {byte:02x}")
                            continue

                        if len(buffer) >= c.FRAME_SIZE:
                            if buffer[0] != c.HEADER:
                                buffer.pop(0)
                                continue
                            frame = buffer[:c.FRAME_SIZE]
                            buffer = buffer[c.FRAME_SIZE:]
                            self._parse_frame(frame)
                            continue
                        break
            except Exception as e:
                if self.running: self.log(f"[RX Error] {e}")
            time.sleep(0.01)

    def _parse_frame(self, frame):
        crc = frame[7]
        calc_crc = self.calculate_crc(frame[:7])

        frame_hex = frame.hex()
        cmd = frame[1]
        data = frame[2:7]

        if crc != calc_crc:
            self.serial_pc_connection.write(bytes([c.NACK]))
            self.log(f"[STM -> PC] CRC ERROR -> NACK | Frame: {frame_hex}")
            return

        self.serial_pc_connection.write(bytes([c.ACK]))
        self.log(f"[STM -> PC] {hex(cmd)} : {frame_hex}")

        if cmd == c.CMD_GAME_STATE:
            if data[0] == c.CMD_GAME_OVER:
                self.handle_game_over_event()
                self.log(f"[STM -> PC] GAME OVER :{hex(cmd)} : {frame_hex}")


        elif cmd == c.CMD_SNAKE_DELTA:
            x1 = data[0]
            y1 = data[1]
            x2 = data[2]
            y2 = data[3]
            delta_flag = data[4]
            if delta_flag == c.CMD_DELTA_MOVE:
                self.head_x, self.head_y = x1, y1
                self.food_x, self.food_y = x2, y2
                self.log(f"[STM] DELTA MOVE: Head({x1},{y1}) x1, x2({x2},{y2})")
            elif delta_flag == c.CMD_DELTA_WALL:
                self.log(f"[STM] DELTA WALL: Wall({x1},{y1}), ({x2},{y2})")
            else:
                self.log(f"[STM] UNKNOWN DELTA FLAG: {delta_flag}")

        elif cmd == c.CMD_SCORE:
            #self.snake_length = (data[0] << 8) | data[1]
            #self.current_score = (data[2] << 8) | data[3]
            self.snake_length = data[0]
            self.current_score = data[1]
            self.log(f"[STM] SCORE: Length {self.snake_length}, Points {self.current_score}")

    def close(self):
        if self.serial_pc_connection.is_open:
            self.serial_pc_connection.close()


if __name__ == "__main__":
    snake = None
    try:
        snake = SnakeProtocol(baudrate=9600)
        while snake.running:
            time.sleep(0.1)
    except KeyboardInterrupt:
        if snake: snake.exit_game()
    finally:
        if snake:
            snake.close()
