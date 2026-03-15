import os
import time

from PySide6.QtCore import QObject, Signal, Slot, QTimer, Property
from serial.tools import list_ports

from snake_protocol import SnakeProtocol


class Backend(QObject):
    headChanged = Signal(int, int, int)
    foodChanged = Signal(int, int, int)

    scoreChanged = Signal(int)
    lenChanged = Signal(int)

    gameOver = Signal(int, int)
    bestChanged = Signal(int)

    logMessage = Signal(str)

    stmConnectedChanged = Signal()
    stmStatusTextChanged = Signal()
    disconnectEventChanged = Signal()

    def __init__(self, port=None, baudrate=9600, poll_ms=20):
        super().__init__()

        self._port = None
        self._baudrate = baudrate
        self.proto = None

        self._desired_wall_level = 0
        self._desired_speed = 1
        self._desired_restriction_on = False

        self._best_score = 0
        self.bestChanged.emit(self._best_score)

        self._expect_best_from_menu = False
        self._startup_best_requested = False

        self._stm_connected = False
        self._stm_status_text = "STM32: not connected"

        self._disconnect_event = 0
        self._suppress_game_over = False
        self._reconnect_handshake_sent = False

        self._last_poll_error_text = ""
        self._last_error_log_ts = 0.0

        self._last_score = None
        self._last_len = None
        self._last_game_over = False

        if self._port is None:
            self._port = self._auto_detect_port()

        self._set_stm_state(False)
        self._need_reconnect_handshake = False

        self.timer = QTimer(self)
        self.timer.setInterval(poll_ms)
        self.timer.timeout.connect(self._poll_state)
        self.timer.start()

        self._poll_state()

    def _auto_detect_port(self) -> str | None:
        try:
            ports = list_ports.comports()
        except Exception:
            return None

        for p in ports:
            desc = (p.description or "").upper()
            if "STM" in desc or "ST" in desc:
                return p.device
        if ports:
            return ports[0].device

        return None

    def _port_present(self) -> bool:
        try:
            return any(p.device == self._port for p in list_ports.comports())
        except Exception:
            return True

    def _disconnect_proto(self):
        try:
            if hasattr(self, "proto") and self.proto:
                self.proto.close()
        except Exception:
            pass

    def _ensure_proto(self) -> bool:
        try:
            self.proto = SnakeProtocol(port=self._port, baudrate=self._baudrate, enable_keyboard=False)
            self._last_score = None
            self._last_len = None
            self._last_game_over = False

            self._apply_config_now()

            if not self._startup_best_requested:
                self._startup_best_requested = True
                self._expect_best_from_menu = True
                try:
                    self.proto.back_to_menu()
                except Exception:
                    self._startup_best_requested = False
                    self._expect_best_from_menu = False
            return True
        except Exception:
            return False

    def _set_stm_state(self, connected: bool):
        prev = self._stm_connected

        new_text = "STM32: connected" if connected else "STM32: not connected"
        changed_conn = (prev != connected)
        changed_text = (self._stm_status_text != new_text)

        self._stm_connected = connected
        self._stm_status_text = new_text

        if changed_conn:
            if changed_conn:
                if not connected:
                    self._set_disconnect_event(1)
                    self._reconnect_handshake_sent = False
                    self._need_reconnect_handshake = True
                    self._last_game_over = False
                else:
                    self._set_disconnect_event(0)
                    self._reconnect_handshake_sent = False

            self.stmConnectedChanged.emit()

        if changed_text:
            self.stmStatusTextChanged.emit()

    @Property(bool, notify=stmConnectedChanged)
    def stmConnected(self) -> bool:
        return self._stm_connected

    @Property(str, notify=stmStatusTextChanged)
    def stmStatusText(self) -> str:
        return self._stm_status_text

    @Property(int, notify=disconnectEventChanged)
    def disconnectEvent(self) -> int:
        return self._disconnect_event

    @Slot()
    def refreshStmStatus(self):
        self._poll_state()

    def _apply_config_now(self):
        try:
            if hasattr(self.proto, "clear_events"):
                self.proto.clear_events()

            self.proto.set_speed(int(self._desired_speed))

            if self._desired_restriction_on:
                self.proto.restriction_on()
            else:
                self.proto.restriction_off()

            self.proto.set_wall(int(self._desired_wall_level))

            self.logMessage.emit(
                f"[PC] Applied config: speed={self._desired_speed}, "
                f"restriction={'ON' if self._desired_restriction_on else 'OFF'}, "
                f"walls={self._desired_wall_level}"
            )
        except Exception as e:
            self.logMessage.emit(f"[PC] apply_config error: {e}")

    def _poll_state(self):
        if (not self._port) or (not self._port_present()):
            if self._stm_connected:
                self._set_stm_state(False)
                self._disconnect_proto()

            detected = self._auto_detect_port()
            if detected and detected != self._port:
                self._port = detected

            if not self._port:
                return

        if not self._stm_connected:
            ok = self._ensure_proto()
            if not ok:
                self._set_stm_state(False)
                return
            self._set_stm_state(True)

        if self._stm_connected and self._need_reconnect_handshake and (not self._reconnect_handshake_sent):
            self._reconnect_handshake_sent = True
            try:
                self._expect_best_from_menu = True
                self.proto.back_to_menu()
                self.logMessage.emit("[PC] Reconnect handshake: back_to_menu() sent")
                self._need_reconnect_handshake = False
            except Exception as e:
                self.logMessage.emit(f"[PC] Reconnect handshake error: {e}")
                self._reconnect_handshake_sent = False

        try:
            events = self.proto.pop_events()
            if not self._stm_connected:
                self._set_stm_state(True)
        except Exception as e:
            if self._stm_connected:
                self._set_stm_state(False)

            err_text = str(e)
            now = time.time()
            if err_text != self._last_poll_error_text or (now - self._last_error_log_ts) > 1.5:
                self._last_poll_error_text = err_text
                self._last_error_log_ts = now
                self.logMessage.emit(f"[PC] Poll error: {e}")
            return

        for evt in events:
            t = evt[0]

            if t == "delta":
                _, hx, hy, x2, y2, flag = evt
                wall = 1 if int(flag) != 0 else 0

                # emit pair in one poll
                self.headChanged.emit(int(hx), int(hy), wall)
                self.foodChanged.emit(int(x2), int(y2), wall)

            elif t == "score":
                _, ln, sc = evt
                sc = int(sc)
                ln = int(ln)

                if self._expect_best_from_menu:
                    self._expect_best_from_menu = False
                    if sc != self._best_score:
                        self._best_score = sc
                        self.bestChanged.emit(self._best_score)
                    continue

                if sc != self._last_score:
                    self._last_score = sc
                    self.scoreChanged.emit(sc)

                if ln != self._last_len:
                    self._last_len = ln
                    self.lenChanged.emit(ln)

            elif t == "state":
                if len(evt) >= 2 and evt[1] == "game_over":
                    if self._disconnect_event == 1 or self._suppress_game_over:
                        continue

                    if not self._last_game_over:
                        self._last_game_over = True
                        sc = int(self._last_score or 0)
                        self.gameOver.emit(sc, self._best_score)
                else:
                    if self._last_game_over:
                        self._last_game_over = False

    def _set_disconnect_event(self, v: int):
        v = 1 if int(v) else 0
        if self._disconnect_event != v:
            self._disconnect_event = v
            self.disconnectEventChanged.emit()

    @Slot()
    def startGame(self):
        self._last_game_over = False
        self._suppress_game_over = False
        try:
            self.proto.start_game(clear_queue=False)
        except Exception as e:
            self.logMessage.emit(f"[PC] startGame error: {e}")

    @Slot()
    def pauseGame(self):
        try:
            self.proto.game_pause()
        except Exception as e:
            self.logMessage.emit(f"[PC] pauseGame error: {e}")

    @Slot()
    def resumeGame(self):
        try:
            self.proto.resume_game()
        except Exception as e:
            self.logMessage.emit(f"[PC] resumeGame error: {e}")

    @Slot()
    def backToMenu(self):
        try:
            self._expect_best_from_menu = True
            self.proto.back_to_menu()
        except Exception as e:
            self.logMessage.emit(f"[PC] backToMenu error: {e}")

    @Slot()
    def exitApp(self):
        try:
            self.proto.exit_game()
        except:
            pass
        self._disconnect_proto()

    @Slot(int)
    def move(self, direction: int):
        try:
            self.proto.move(int(direction))
        except Exception as e:
            self.logMessage.emit(f"[PC] move error: {e}")

    @Slot(int)
    def setDebugMode(self, flag: int):
        try:
            self.proto.set_debug_mode(int(flag))
        except Exception as e:
            self.logMessage.emit(f"[PC] setDebugMode error: {e}")

    @Slot(int)
    def setSpeed(self, speed_level: int):
        self._desired_speed = int(speed_level)
        self.logMessage.emit(f"[PC] QML -> setSpeed({self._desired_speed})")
        try:
            self.proto.set_speed(self._desired_speed)
        except Exception as e:
            self.logMessage.emit(f"[PC] setSpeed error: {e}")

    @Slot()
    def config(self):
        self.logMessage.emit("[PC] QML -> config()")
        try:
            self.proto.config()
        except Exception as e:
            self.logMessage.emit(f"[PC] config error: {e}")

    @Slot()
    def restrictionOn(self):
        self._desired_restriction_on = True
        self.logMessage.emit("[PC] QML -> restrictionOn()")
        try:
            self.proto.restriction_on()
        except Exception as e:
            self.logMessage.emit(f"[PC] restrictionOn error: {e}")

    @Slot()
    def restrictionOff(self):
        self._desired_restriction_on = False
        self.logMessage.emit("[PC] QML -> restrictionOff()")
        try:
            self.proto.restriction_off()
        except Exception as e:
            self.logMessage.emit(f"[PC] restrictionOff error: {e}")

    @Slot(int)
    def walls(self, wall_level: int):
        self._desired_wall_level = int(wall_level)
        self.logMessage.emit(f"[PC] QML -> walls({self._desired_wall_level})")
        try:
            self.proto.set_wall(self._desired_wall_level)
        except Exception as e:
            self.logMessage.emit(f"[PC] walls error: {e}")