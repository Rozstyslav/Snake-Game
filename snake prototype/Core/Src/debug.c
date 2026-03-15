#include "debug.h"
#include "uart_protocol.h"
#include "game_config.h"
#include "main.h"
#include "led.h"
#include <stdlib.h>

// Для прямого доступу до UART (потрібно для інжекції помилок CRC)
extern UART_HandleTypeDef huart1;

static uint8_t current_mode = DEBUG_OFF;

// ─── Сценарій відтворення ──────────────────────────────────────────────
// Вибір: використовувати зовнішній файл сценарію чи вбудований
#ifdef USE_EXTERNAL_SCENARIO
#include "scenario_data.h"
static const ScenarioStep_t* current_scenario = scenario_steps;
static uint16_t scenario_len = SCENARIO_STEP_COUNT;   // повна кількість кроків (включно з очікуваннями)
#else
// Вбудований сценарій (за замовчуванням)
typedef struct {
    uint8_t cmd;
    uint8_t payload[5];
    uint32_t delay_ms;
} ScenarioStep_t;

static const ScenarioStep_t scenario_1[] = {
    {CMD_DELTA, {10, 10,  5,  5, 0}, 500},
    {CMD_DELTA, {11, 10,  5,  5, 0}, 500},
    {CMD_DELTA, {12, 10,  5,  5, 0}, 500},
    {CMD_DELTA, {12, 11,  5,  5, 0}, 500},
    {CMD_DELTA, {12, 12,  5,  5, 0}, 500},
    {CMD_DELTA, {11, 12,  5,  5, 0}, 500},
    {CMD_DELTA, {10, 12,  5,  5, 0}, 500},
    {CMD_DELTA, {10, 11,  5,  5, 0}, 500},
    {CMD_SCORE, {5, 0, 0, 0, 0}, 500},
    {0, {0, 0, 0, 0, 0}, 0}  // термінатор (cmd=0, delay=0)
};
static const ScenarioStep_t* current_scenario = scenario_1;
// Кількість кроків без термінатора
static uint16_t scenario_len = sizeof(scenario_1) / sizeof(scenario_1[0]) - 1;
#endif

static uint16_t scenario_index = 0;
static uint32_t last_scenario_time = 0;

// ─── Випадкові пакети ──────────────────────────────────────────────────
static uint32_t last_random_time = 0;
#define RANDOM_INTERVAL_MS 500

// ─── Інжекція помилок ──────────────────────────────────────────────────
static uint32_t last_error_time = 0;
#define ERROR_INJECT_INTERVAL_MS 1000

void Debug_Init(void) {
    // srand() має бути викликаний у main
}

void Debug_SetMode(uint8_t mode) {
    if (mode <= DEBUG_STEP_MODE) {
        current_mode = mode;
        // Скидання внутрішнього стану для всіх режимів
        scenario_index = 0;
        last_scenario_time = 0;           // перший крок відбудеться негайно
        last_random_time = 0;
        last_error_time = 0;
        LED_UpdateModes();
        LED_IndicateDebugMode(mode);       // візуальна індикація нового режиму
    }
}

uint8_t Debug_GetMode(void) {
    return current_mode;
}

// ─── Функції для різних режимів ────────────────────────────────────────

static void send_random_packet(void) {
    uint8_t payload[5];
    if (rand() % 10 == 0) {
        payload[0] = BOARD_WIDTH + (rand() % 10);
        payload[1] = BOARD_HEIGHT + (rand() % 10);
    }
    else {
        payload[0] = rand() % BOARD_WIDTH;
        payload[1] = rand() % BOARD_HEIGHT;
    }
    if (rand() % 10 == 0) {
        payload[2] = rand() % BOARD_WIDTH;
        payload[3] = rand() % BOARD_HEIGHT;
    }
    else {
        payload[2] = 0;
        payload[3] = 0;
    }
    payload[4] = (rand() % 5 == 0) ? 1 : 0;
    UART_SendPacket(CMD_DELTA, payload, 5);
}

static void send_scenario_step(uint32_t now) {
    // Якщо дійшли до кінця – починаємо спочатку
    if (scenario_index >= scenario_len) {
        scenario_index = 0;
        // Не скидаємо час, щоб перший крок виконався з власною затримкою
        // Просто повертаємось і даємо наступному виклику обробити перший крок
        return;
    }

    const ScenarioStep_t* step = &current_scenario[scenario_index];

    // Перевіряємо, чи минула затримка для поточного кроку
    if (now - last_scenario_time < step->delay_ms) {
        return; // ще не час
    }

    // Якщо це крок з командою (cmd != 0) – відправляємо пакет
    if (step->cmd != 0) {
        UART_SendPacket(step->cmd, step->payload, 5);
    }
    // Якщо cmd == 0 – це просто пауза, нічого не відправляємо

    // Переходимо до наступного кроку
    scenario_index++;
    last_scenario_time = now; // фіксуємо час завершення цього кроку
}

static void inject_error(uint32_t now) {
    if (now - last_error_time >= ERROR_INJECT_INTERVAL_MS) {
        // Формуємо пакет вручну
        uint8_t packet[8];
        packet[0] = 0xAA;
        packet[1] = CMD_DELTA;
        packet[2] = 0;  // payload all zero
        packet[3] = 0;
        packet[4] = 0;
        packet[5] = 0;
        packet[6] = 0;
        // Обчислюємо правильний CRC
        packet[7] = 0;
        for (int i = 0; i < 7; i++) {
            packet[7] ^= packet[i];
        }
        // Тепер псуємо CRC (наприклад, інвертуємо)
        packet[7] ^= 0xFF;

        // Відправляємо безпосередньо через HAL (оминаючи UART_SendPacket)
        HAL_UART_Transmit(&huart1, packet, 8, UART_PACKET_TIMEOUT);

        last_error_time = now;
    }
}

// ─── Головна функція обробки ───────────────────────────────────────────

void Debug_Process(uint32_t now) {
    switch (current_mode) {
    case DEBUG_RANDOM_PACKETS:
        if (now - last_random_time >= RANDOM_INTERVAL_MS) {
            send_random_packet();
            last_random_time = now;
        }
        break;

    case DEBUG_SCENARIO_1:
        send_scenario_step(now);
        break;

    case DEBUG_ERROR_INJECTION:
        inject_error(now);
        break;

    case DEBUG_STEP_MODE:      // нічого автоматичного
    case DEBUG_SILENT_FLAG:
    case DEBUG_LOOPBACK:       // обробляється в Debug_OnCommand
    case DEBUG_OFF:
    default:
        break;
    }
}

void Debug_OnCommand(uint8_t cmd, uint8_t* payload) {
    if (current_mode == DEBUG_LOOPBACK) {
        uint8_t echo[5] = { cmd, payload[0], payload[1], payload[2], payload[3] };
        UART_SendPacket(CMD_DELTA, echo, 5);
    }
}