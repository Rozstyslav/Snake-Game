# Snake on STM32F0 — Backend Developer Guide (`game_engine.c`)

## Introduction

You are responsible for implementing the **Snake game logic** on the STM32F0 microcontroller.
All surrounding infrastructure (UART, button handling, LED control, debug modes, high-score
persistence) is already implemented and tested. Your only task is to write **`game_engine.c`**.

> **Critical constraint:**
> You **must not modify any other files**, including:
> `main.c`, `uart_protocol.c / .h`, `game_io.c / .h`, `debug.c / .h`,
> `led.c / .h`, `button.c / .h`, `game_config.h`, `main.h`, or any STM32 system files.
>
> All interaction with hardware and the PC happens **exclusively through functions
> declared in `game_io.h`**. Driver files are a black box — they give you a stable,
> unchanging interface. If you need functionality that is missing from `game_io.h`,
> formulate a clear request to the driver developer (see the [LLM Prompt](#7-llm-prompt-template) section).

---

## 1. `game_config.h` — Shared Constants

`game_config.h` is the single source of truth for all tuneable parameters.
**Read it; never hardcode the values it defines.**

### Board geometry

```c
#define BOARD_WIDTH          50
#define BOARD_HEIGHT         50
#define START_HEAD_X         (BOARD_WIDTH  / 2)   // 25
#define START_HEAD_Y         (BOARD_HEIGHT / 2)   // 25
```

### Snake limits

```c
#define MAX_SNAKE_LEN        100   // hard cap on segment count
#define FOOD_FIND_MAX_ATTEMPTS 1000 // give up placing food after this many retries
```

### Game speed (milliseconds per step)

These are the concrete ms values that `GameIO_GetSpeed()` returns.
You do not need to define them yourself — they are here for reference.

```c
#define SPEED_MS_SLOW        400   // SPEED_SLOW   (0x00)
#define SPEED_MS_MEDIUM      200   // SPEED_MEDIUM (0x01) — default
#define SPEED_MS_FAST        100   // SPEED_FAST   (0x02)
```

### Level / wall selection

```c
#define GAME_LEVEL_MAX       6     // highest accepted level index (0–6)
```

### Debug mode count (for cycling)

```c
#define DEBUG_MODE_COUNT     7     // DEBUG_OFF(0) … DEBUG_STEP_MODE(6)
```

### Button timing (for reference only — handled by `button.c`)

```c
#define BUTTON_DEBOUNCE_MS        50
#define BUTTON_SHORT_PRESS_MS    800
#define BUTTON_LONG_PRESS_MS    2000
```

### LED timing (for reference only — handled by `led.c`)

```c
#define LED_SLOW_BLINK_HALF    500   // ms, 1 Hz
#define LED_FAST_BLINK_HALF    125   // ms, 4 Hz
```

### UART timeouts (for reference only — handled by `uart_protocol.c`)

```c
#define UART_ACK_TIMEOUT        10   // ms
#define UART_PACKET_TIMEOUT    100   // ms
```

### High-score Flash address (for reference only — handled by `game_io.c`)

```c
#define HIGHSCORE_FLASH_ADDR  0x0800FC00u   // last 1 KB page of 64 KB Flash
```

---

## 2. Available API (`game_io.h`)

### 2.1 Game state

```c
uint8_t GameIO_GetState(void);
void    GameIO_SetState(uint8_t state);
```

State values (defined in `uart_protocol.h`):

| Constant | Value | Meaning |
|---|---|---|
| `STATE_MENU` | 0 | Main menu, waiting for start |
| `STATE_PLAYING` | 1 | Game active |
| `STATE_PAUSE` | 2 | Game paused |
| `STATE_GAMEOVER` | 3 | Game over, waiting for return to menu |
| `STATE_SLEEP` | 4 | Low-power sleep mode |

`GameIO_SetState` automatically updates LED patterns (the mapping is baked into `led.c`).
Call it **only on genuine state transitions** — not for internal flags.

When entering `STATE_MENU` the driver automatically sends the current high score to the PC
(`snake_len = 0`, `score = high_score`). You do not need to do anything for this.

### 2.2 Configuration (set from PC via `CMD_CONFIG`)

```c
uint16_t GameIO_GetSpeed(void);           // returns SPEED_MS_SLOW/MEDIUM/FAST
uint8_t  GameIO_GetWallDifficulty(void);  // returns the selected level index (0–5)
uint8_t  GameIO_GetBoundaryMode(void);    // BOUNDARY_WRAP (0) or BOUNDARY_GAMEOVER (1)
uint8_t  GameIO_GetSelectedLevel(void);   // same as GetWallDifficulty, preferred alias
```

> **Important change from earlier versions:**
> `GameIO_GetWallDifficulty()` now returns the **level index (0–5)**, not a difficulty tier.
> The level index maps directly to a layout built by `game_engine.c` itself via `Setup_Level()`.
> The values `WALLS_NONE / WALLS_EASY / WALLS_HARD` from `uart_protocol.h` are still
> transmitted by the PC but are translated to level indices in `game_io.c`.

These values are updated automatically when `CMD_CONFIG` is received from the PC.

### 2.3 Sending data to the PC

```c
void GameIO_SendDelta(uint8_t head_x, uint8_t head_y,
                      uint8_t x2,     uint8_t y2,
                      uint8_t wall_flag);

void GameIO_SendScore(uint8_t snake_len, uint16_t score);
void GameIO_SendState(uint8_t state);
```

#### `GameIO_SendDelta` — packet semantics

| `wall_flag` | `(head_x, head_y)` | `(x2, y2)` |
|---|---|---|
| `0` — normal move | new head position | new food position (if food eaten) **or** tail cell to delete |
| `1` — wall placement | first wall cell | second wall cell |

When `wall_flag == 1` the movement semantics are ignored; both coordinate pairs are treated as
wall cells to mark on the PC display.

#### `GameIO_SendScore`

Call this every time the score or snake length changes (in practice: on every food pickup).

```
payload[0] = snake_len   (1 byte)
payload[1] = score_low   (LSB of score)
payload[2] = score_high  (MSB of score)
payload[3..4] = 0
```

#### `GameIO_SendState`

The driver already calls this automatically when you call `GameIO_SetState`, so you rarely
need to call `GameIO_SendState` directly.

### 2.4 High-score submission

```c
void GameIO_SubmitScore(uint16_t score);
```

Call this **before** transitioning to `STATE_GAMEOVER`. The driver compares the value with
the stored high score and writes to Flash only if the new score is higher.

```c
// correct pattern
GameIO_SubmitScore(score);
GameIO_SetState(STATE_GAMEOVER);
GameIO_SendState(STATE_GAMEOVER);
```

### 2.5 Level selection

```c
void    GameIO_SetSelectedLevel(uint8_t level);   // 0 ≤ level ≤ GAME_LEVEL_MAX
uint8_t GameIO_GetSelectedLevel(void);
```

The driver calls `GameIO_SetSelectedLevel` when `CMD_CONFIG / CONFIG_WALLS` is received.
Your engine reads it with `GameIO_GetSelectedLevel()` at the start of each game.

### 2.6 Debug mode

```c
uint8_t GameIO_GetDebugMode(void);
```

Returns the current debug mode (0–6):

| Value | Constant | Behaviour |
|---|---|---|
| 0 | `DEBUG_OFF` | Normal |
| 1 | `DEBUG_RANDOM_PACKETS` | Driver sends random deltas |
| 2 | `DEBUG_SILENT_FLAG` | Flag only, LED may indicate |
| 3 | `DEBUG_SCENARIO_1` | Pre-defined scenario playback |
| 4 | `DEBUG_LOOPBACK` | Driver echoes received commands |
| 5 | `DEBUG_ERROR_INJECTION` | Driver injects bad CRC packets |
| 6 | `DEBUG_STEP_MODE` | `Game_Update()` called only on button short-press |

`DEBUG_STEP_MODE` is handled entirely by `main.c` / `button.c` — your code does not need
to change behaviour; it simply executes one step each time it is called.

### 2.7 Command callback registration

```c
typedef void (*GameIO_CommandCallback)(uint8_t cmd, uint8_t *payload);
void GameIO_RegisterCallback(GameIO_CommandCallback cb);
```

Register your handler once (e.g. at the start of `game_engine.c`).
The driver **already handles** `CMD_CONFIG`, `CMD_DEBUG`, and `CMD_SET_STATE` internally;
your callback still receives all commands and can react to them additionally.

---

## 3. Functions You Must Implement (`game_engine.h`)

```c
uint16_t Game_GetSpeedMs(void);
void     Game_Update(void);
void     Game_HandleCommand(uint8_t cmd, uint8_t *payload);
void     Game_Start(void);
```

### 3.1 `Game_GetSpeedMs`

```c
uint16_t Game_GetSpeedMs(void) {
    return GameIO_GetSpeed();
}
```

### 3.2 `Game_Start`

Called by `main.c` (button short-press in MENU state) and by `Game_HandleCommand` when
`CMD_SET_STATE → STATE_PLAYING` is received from a MENU or GAMEOVER state.

Responsibilities:
- Reset all internal game state (snake, food, score, direction).
- Call `Setup_Level(GameIO_GetSelectedLevel())` to populate the collision map and send wall
  packets to the PC.
- Generate initial food.
- Call `GameIO_SetState(STATE_PLAYING)`.
- Send initial `GameIO_SendDelta` and `GameIO_SendScore`.

### 3.3 `Game_HandleCommand`

```c
void Game_HandleCommand(uint8_t cmd, uint8_t *payload) {
    switch (cmd) {
        case CMD_MOVE: {
            // validate direction; reject 180° U-turns
            // store as next_dir (applied at start of next Game_Update)
            break;
        }
        case CMD_SET_STATE:
            if (payload[0] == STATE_PLAYING &&
                (GameIO_GetState() == STATE_MENU ||
                 GameIO_GetState() == STATE_GAMEOVER)) {
                Game_Start();   // driver already changed state; start game logic
            } else {
                GameIO_SetState(payload[0]);
                GameIO_SendState(payload[0]);
            }
            break;
        case CMD_CONFIG:
            // driver already updated config values — no action required
            break;
        default:
            break;
    }
}
```

### 3.4 `Game_Update`

Called from `main.c` at the interval returned by `Game_GetSpeedMs()`.
Only executes if `GameIO_GetState() == STATE_PLAYING` and a direction has been set.

Full sequence per step:
1. Apply `next_dir` → `curr_dir`.
2. Compute `next_head` based on `curr_dir`.
3. Apply boundary logic (`BOUNDARY_WRAP` or `BOUNDARY_GAMEOVER`).
4. Check collision with walls (`collision_map[x][y]`).
5. Check self-collision (skip the last tail cell if not eating).
6. If any collision: call `GameIO_SubmitScore(score)`, set `STATE_GAMEOVER`, return.
7. Determine `ate_food`.
8. Shift the snake array; update head.
9. If `ate_food`: grow, increment score, generate new food, send `SendDelta` with new food,
   send `SendScore`.
10. If not eating: send `SendDelta` with old tail to delete.

---

## 4. Recommended Internal Data Structures

Declare everything as `static` in `game_engine.c`:

```c
#include "game_engine.h"
#include "game_io.h"
#include "game_config.h"
#include <stdlib.h>
#include <string.h>

/* Snake body */
typedef struct { int8_t x; int8_t y; } Point_t;
static Point_t snake[MAX_SNAKE_LEN];   // MAX_SNAKE_LEN from game_config.h
static uint8_t snake_len = 0;

/* Direction */
typedef enum { DIR_UP=0, DIR_DOWN=1, DIR_LEFT=2, DIR_RIGHT=3, DIR_NONE=255 } Direction_t;
static Direction_t curr_dir = DIR_NONE;
static Direction_t next_dir = DIR_NONE;

/* Food */
static Point_t food;

/* Score */
static uint16_t score = 0;

/* Wall collision map */
static uint8_t collision_map[BOARD_WIDTH][BOARD_HEIGHT];
```

> Use `DIR_NONE` as the initial value of `next_dir` so that the snake does not move until
> the first `CMD_MOVE` is received. `Game_Update` should return immediately if
> `next_dir == DIR_NONE`.

---

## 5. Level System

The engine is responsible for building the wall layout. The PC learns about walls through
`GameIO_SendDelta` calls with `wall_flag = 1`.

```c
static void Add_Wall_Pair(int8_t x1, int8_t y1, int8_t x2, int8_t y2) {
    if (x1 >= 0 && x1 < BOARD_WIDTH  && y1 >= 0 && y1 < BOARD_HEIGHT)
        collision_map[x1][y1] = 1;
    if (x2 >= 0 && x2 < BOARD_WIDTH  && y2 >= 0 && y2 < BOARD_HEIGHT)
        collision_map[x2][y2] = 1;
    GameIO_SendDelta((uint8_t)x1, (uint8_t)y1, (uint8_t)x2, (uint8_t)y2, 1);
}

static void Setup_Level(uint8_t level) {
    memset(collision_map, 0, sizeof(collision_map));
    switch (level) {
        case 0: /* empty field */           break;
        case 1: /* vertical corridors */    /* … */ break;
        case 2: /* concentric rings */      /* … */ break;
        case 3: /* corner crosses */        /* … */ break;
        case 4: /* maze */                  /* … */ break;
        case 5: /* symmetric C-shapes */    /* … */ break;
        default: break;
    }
}
```

All level geometry is self-contained in `game_engine.c`. The PC side stores whatever
wall packets it receives; there is no need to transmit level metadata separately.

---

## 6. High-Score Pattern

```c
// At every game-over transition — before SetState:
GameIO_SubmitScore(score);
GameIO_SetState(STATE_GAMEOVER);
GameIO_SendState(STATE_GAMEOVER);
```

The driver writes to Flash only when the new score exceeds the stored value.
On `GameIO_SetState(STATE_MENU)` the driver automatically broadcasts the high score
to the PC as `SendScore(0, high_score)`.

---

## 7. LLM Prompt Template

If you are asking an LLM to help write `game_engine.c`, prepend the following context:

---

> I am working on a Snake game for STM32F0. The infrastructure (UART, button, LEDs,
> debug modes, high-score storage) is fully implemented. My only task is to write
> `game_engine.c`. Here is the available API from `game_io.h`:
>
> - `GameIO_GetState()` / `GameIO_SetState(state)` — states: `STATE_MENU`, `STATE_PLAYING`,
>   `STATE_PAUSE`, `STATE_GAMEOVER`, `STATE_SLEEP`
> - `GameIO_GetSpeed()` — returns 100 / 200 / 400 ms
> - `GameIO_GetSelectedLevel()` — returns level index 0–5
> - `GameIO_GetBoundaryMode()` — `BOUNDARY_WRAP` or `BOUNDARY_GAMEOVER`
> - `GameIO_SendDelta(head_x, head_y, x2, y2, wall_flag)` — see delta semantics
> - `GameIO_SendScore(snake_len, score)` — both arguments required
> - `GameIO_SendState(state)` — rarely needed, driver calls it automatically
> - `GameIO_SubmitScore(score)` — call before every game-over transition
> - `GameIO_GetDebugMode()` — returns 0–6
> - `GameIO_RegisterCallback(cb)` — register command handler
>
> Constants in `game_config.h`: `BOARD_WIDTH` (50), `BOARD_HEIGHT` (50),
> `START_HEAD_X/Y` (25), `MAX_SNAKE_LEN` (100), `FOOD_FIND_MAX_ATTEMPTS` (1000),
> `GAME_LEVEL_MAX` (6), `DEBUG_MODE_COUNT` (7).
>
> Functions I must implement (`game_engine.h`):
> `Game_GetSpeedMs()`, `Game_Update()`, `Game_HandleCommand()`, `Game_Start()`.
>
> **Do not suggest changes to any other file.** If you need a function that does not
> exist in `game_io.h`, state it as a request to the driver developer instead.

---

## 8. Testing with the Simulator

The simulator (`sim_main.c` + `ui.c`) runs on Windows / Linux and uses the same
`game_engine.c` compiled with stub HAL headers — no changes to the engine are needed.

### Simulator commands

| Command | Effect |
|---|---|
| `play` | Sends `CMD_SET_STATE → STATE_PLAYING` |
| `up / down / left / right` | Sends `CMD_MOVE` |
| `level 0`…`level 5` | Sends `CMD_CONFIG / CONFIG_WALLS` with the level index |
| `speed slow / medium / fast` | Sends `CMD_CONFIG / CONFIG_SPEED` |
| `wrap` / `nowrap` | Sends `CMD_CONFIG / CONFIG_BOUNDARY` |
| `dbg 0`…`dbg 6` | Sends `CMD_DEBUG` |
| `btn short / medium / long` | Simulates a timed button press |
| `walls` | Lists all received wall packets |
| `map` | Renders an ASCII wall map |
| `help` | Full command list |

### Recommended workflow

1. Write `game_engine.c`.
2. Run the simulator; use `play` + movement commands to verify DELTA packets in the right panel.
3. Use `level 2` or `level 5` to verify wall generation and collision.
4. Use `btn short` in `STATE_PLAYING` to verify pause → resume.
5. Use `dbg 6` + `btn short` to step through the game one frame at a time.
6. Check that `SCORE` packets appear with correct `len` and `val` on every food pickup.
7. Verify that the high score is broadcast on return to `MENU`.
8. Flash the binary to the STM32 board only after the simulator passes.

---

## 9. Summary Checklist

- [ ] Only `game_engine.c` is modified
- [ ] All board dimensions come from `game_config.h` (`BOARD_WIDTH`, `BOARD_HEIGHT`, etc.)
- [ ] `MAX_SNAKE_LEN` is taken from `game_config.h`, not redefined locally
- [ ] `next_dir` initialised to `DIR_NONE`; movement blocked until first `CMD_MOVE`
- [ ] 180° U-turn rejected in `Game_HandleCommand / CMD_MOVE`
- [ ] `collision_map` populated by `Setup_Level()` at game start
- [ ] `GameIO_SubmitScore(score)` called before every game-over transition
- [ ] `GameIO_SendScore(snake_len, score)` sends **both** arguments on food pickup
- [ ] `GameIO_SendDelta` with `wall_flag=1` sent for each wall pair during level setup
- [ ] `GameIO_SendDelta` with `wall_flag=0`: second coords = **new food** (ate) or **old tail** (moved)
- [ ] Self-collision check skips last tail segment when not eating
- [ ] Simulator tested before flashing to hardware
