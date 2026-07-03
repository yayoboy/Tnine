#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pin mapping (Raspberry Pi Pico / RP2040)
// ---------------------------------------------------------------------------

// I2C0 – OLED 0.42" (SSD1306 72x40) – anteprima carattere
constexpr uint8_t PIN_PREVIEW_SDA = 4;
constexpr uint8_t PIN_PREVIEW_SCL = 5;

// I2C1 – OLED 128x64 (SSD1306) – interfaccia utente
constexpr uint8_t PIN_UI_SDA = 6;
constexpr uint8_t PIN_UI_SCL = 7;

// UART1 – modulo seriale Meshtastic (mode PROTO, client API protobuf)
// PIN_MESH_TX (RP2040 TX) -> RXD del nodo Meshtastic
// PIN_MESH_RX (RP2040 RX) <- TXD del nodo Meshtastic
constexpr uint8_t PIN_MESH_TX = 8;
constexpr uint8_t PIN_MESH_RX = 9;
constexpr uint32_t MESH_BAUD = 38400;   // default del serial module Meshtastic

// Canale su cui trasmettere (0 = primario).
constexpr uint32_t MESH_CHANNEL = 0;
// Heartbeat per tenere viva la sessione client API.
constexpr uint32_t MESH_HEARTBEAT_MS = 60000;
// Reinvio del want_config_id finché il nodo non completa l'handshake.
constexpr uint32_t MESH_CONFIG_RETRY_MS = 5000;

// Tastierino a matrice 4 righe x 3 colonne
//   1 2 3
//   4 5 6
//   7 8 9
//   * 0 #
constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 3;
constexpr uint8_t KEYPAD_ROW_PINS[KEYPAD_ROWS] = {10, 11, 12, 13};
constexpr uint8_t KEYPAD_COL_PINS[KEYPAD_COLS] = {14, 15, 16};

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

constexpr uint32_t KEY_DEBOUNCE_MS   = 20;   // antirimbalzo tastierino
constexpr uint32_t KEY_LONGPRESS_MS  = 600;  // soglia pressione lunga (* e #)
constexpr uint32_t T9_MULTITAP_MS    = 800;  // timeout conferma carattere multi-tap

// ---------------------------------------------------------------------------
// Limiti
// ---------------------------------------------------------------------------

// Il payload testo di Meshtastic è ~237 byte: teniamoci un margine.
constexpr size_t MESSAGE_MAX_LEN = 200;
