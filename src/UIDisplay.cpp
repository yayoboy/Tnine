#include "UIDisplay.h"

#include <U8g2lib.h>
#include <Wire.h>

#include "T9Engine.h"
#include "config.h"

// _2ND_HW_I2C usa Wire1 sul core arduino-pico.
static U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C u8g2(U8G2_R0);

// Font 6x12 -> 21 colonne su 128 px.
static constexpr int COLS = 21;
static constexpr int COMPOSE_LINES = 3;

void UIDisplay::begin() {
    Wire1.setSDA(PIN_UI_SDA);
    Wire1.setSCL(PIN_UI_SCL);
    u8g2.begin();
}

void UIDisplay::render(const T9Engine& t9, const String& lastRx, const String& status) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);

    // --- Barra di stato -----------------------------------------------------
    char bar[32];
    snprintf(bar, sizeof(bar), "%s %3u/%u", t9.modeLabel(),
             static_cast<unsigned>(t9.text().length()),
             static_cast<unsigned>(MESSAGE_MAX_LEN));
    u8g2.drawStr(0, 9, bar);
    if (status.length() > 0) {
        int w = u8g2.getStrWidth(status.c_str());
        u8g2.drawStr(128 - w, 9, status.c_str());
    }
    u8g2.drawHLine(0, 11, 128);

    // --- Area di composizione: coda del testo + candidato + cursore ---------
    String compose = t9.text();
    if (t9.hasPending()) {
        compose += t9.candidate();
    }
    compose += '_';  // cursore

    // Mostra solo le ultime COMPOSE_LINES righe.
    int totalLines = (compose.length() + COLS - 1) / COLS;
    int firstLine = max(0, totalLines - COMPOSE_LINES);
    int y = 22;
    for (int line = firstLine; line < totalLines; ++line) {
        String chunk = compose.substring(line * COLS,
                                         min((line + 1) * COLS, (int)compose.length()));
        u8g2.drawStr(0, y, chunk.c_str());
        y += 12;
    }

    // --- Ultimo messaggio ricevuto -------------------------------------------
    u8g2.drawHLine(0, 52, 128);
    String rx = lastRx.length() > 0 ? ("RX: " + lastRx) : String("RX: -");
    if ((int)rx.length() > COLS) {
        rx = rx.substring(0, COLS - 1) + ">";
    }
    u8g2.drawStr(0, 63, rx.c_str());

    u8g2.sendBuffer();
}
