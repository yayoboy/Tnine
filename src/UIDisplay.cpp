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

// Byte iniziali di un carattere UTF-8 (esclude i byte di continuazione).
static bool isUtf8Start(char c) {
    return (static_cast<uint8_t>(c) & 0xC0) != 0x80;
}

// Numero di caratteri (codepoint) in una stringa UTF-8.
static int utf8Length(const String& s) {
    int n = 0;
    for (unsigned i = 0; i < s.length(); ++i) {
        if (isUtf8Start(s.charAt(i))) ++n;
    }
    return n;
}

// Indice del byte che inizia il carattere numero `glyphIdx`.
static unsigned utf8ByteIndex(const String& s, int glyphIdx) {
    int n = 0;
    for (unsigned i = 0; i < s.length(); ++i) {
        if (isUtf8Start(s.charAt(i))) {
            if (n == glyphIdx) return i;
            ++n;
        }
    }
    return s.length();
}

void UIDisplay::begin() {
    Wire1.setSDA(PIN_UI_SDA);
    Wire1.setSCL(PIN_UI_SCL);
    u8g2.begin();
    u8g2.enableUTF8Print();
}

void UIDisplay::render(const T9Engine& t9, const String& lastRx,
                       const String& status, const String& linkLabel) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_te);

    // --- Barra di stato -----------------------------------------------------
    char bar[32];
    snprintf(bar, sizeof(bar), "%s %3u/%u", t9.modeLabel(),
             static_cast<unsigned>(t9.text().length()),
             static_cast<unsigned>(MESSAGE_MAX_LEN));
    u8g2.drawStr(0, 9, bar);

    // A destra: lo stato transitorio (inviato/consegnato/…) ha priorità
    // sull'etichetta del collegamento.
    const String& right = status.length() > 0 ? status : linkLabel;
    if (right.length() > 0) {
        int w = u8g2.getUTF8Width(right.c_str());
        u8g2.drawUTF8(128 - w, 9, right.c_str());
    }
    u8g2.drawHLine(0, 11, 128);

    // --- Area di composizione: coda del testo + candidato + cursore ---------
    String compose = t9.text();
    compose += t9.candidate();
    compose += '_';  // cursore

    // A capo per caratteri (non per byte: gli accenti sono multi-byte).
    int totalGlyphs = utf8Length(compose);
    int totalLines = (totalGlyphs + COLS - 1) / COLS;
    int firstLine = max(0, totalLines - COMPOSE_LINES);
    int y = 22;
    for (int line = firstLine; line < totalLines; ++line) {
        unsigned from = utf8ByteIndex(compose, line * COLS);
        unsigned to = utf8ByteIndex(compose, (line + 1) * COLS);
        String chunk = compose.substring(from, to);
        u8g2.drawUTF8(0, y, chunk.c_str());
        y += 12;
    }

    // --- Ultimo messaggio ricevuto -------------------------------------------
    u8g2.drawHLine(0, 52, 128);
    String rx = lastRx.length() > 0 ? lastRx : String("-");
    if (utf8Length(rx) > COLS) {
        rx = rx.substring(0, utf8ByteIndex(rx, COLS - 1)) + ">";
    }
    u8g2.drawUTF8(0, 63, rx.c_str());

    u8g2.sendBuffer();
}
