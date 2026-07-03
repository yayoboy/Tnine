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
static constexpr int LIST_ROWS = 4;
static constexpr int DETAIL_ROWS = 4;

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

// Tronca a maxGlyphs caratteri aggiungendo '>' se necessario.
static String truncated(const String& s, int maxGlyphs) {
    if (utf8Length(s) <= maxGlyphs) return s;
    return s.substring(0, utf8ByteIndex(s, maxGlyphs - 1)) + ">";
}

void UIDisplay::begin() {
    Wire1.setSDA(PIN_UI_SDA);
    Wire1.setSCL(PIN_UI_SCL);
    u8g2.begin();
    u8g2.enableUTF8Print();
}

void UIDisplay::renderCompose(const T9Engine& t9, const String& lastRx,
                              const String& statusRight) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_te);

    // --- Barra di stato -----------------------------------------------------
    char bar[32];
    snprintf(bar, sizeof(bar), "%s %3u/%u", t9.modeLabel(),
             static_cast<unsigned>(t9.text().length()),
             static_cast<unsigned>(MESSAGE_MAX_LEN));
    u8g2.drawStr(0, 9, bar);

    if (statusRight.length() > 0) {
        int w = u8g2.getUTF8Width(statusRight.c_str());
        u8g2.drawUTF8(128 - w, 9, statusRight.c_str());
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
    u8g2.drawUTF8(0, 63, truncated(rx, COLS).c_str());

    u8g2.sendBuffer();
}

void UIDisplay::renderList(const char* title, const String* items, int count,
                           int selected) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_te);

    u8g2.drawUTF8(0, 9, title);
    if (count > LIST_ROWS) {
        char pos[12];
        snprintf(pos, sizeof(pos), "%d/%d", selected + 1, count);
        int w = u8g2.getStrWidth(pos);
        u8g2.drawStr(128 - w, 9, pos);
    }
    u8g2.drawHLine(0, 11, 128);

    // Finestra di scorrimento intorno alla voce selezionata.
    int first = 0;
    if (selected >= LIST_ROWS) first = selected - LIST_ROWS + 1;
    if (first > count - LIST_ROWS) first = max(0, count - LIST_ROWS);

    int y = 23;
    for (int i = first; i < count && i < first + LIST_ROWS; ++i) {
        if (i == selected) {
            u8g2.drawBox(0, y - 10, 128, 13);
            u8g2.setDrawColor(0);
        }
        u8g2.drawUTF8(2, y, truncated(items[i], COLS - 1).c_str());
        u8g2.setDrawColor(1);
        y += 13;
    }

    u8g2.sendBuffer();
}

void UIDisplay::renderDetail(const char* title, const String& text, int scrollLine) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_te);

    u8g2.drawUTF8(0, 9, title);
    int total = detailLines(text);
    if (total > DETAIL_ROWS) {
        char pos[16];
        snprintf(pos, sizeof(pos), "%d-%d/%d", scrollLine + 1,
                 min(scrollLine + DETAIL_ROWS, total), total);
        int w = u8g2.getStrWidth(pos);
        u8g2.drawStr(128 - w, 9, pos);
    }
    u8g2.drawHLine(0, 11, 128);

    int y = 23;
    for (int line = scrollLine;
         line < total && line < scrollLine + DETAIL_ROWS; ++line) {
        unsigned from = utf8ByteIndex(text, line * COLS);
        unsigned to = utf8ByteIndex(text, (line + 1) * COLS);
        u8g2.drawUTF8(0, y, text.substring(from, to).c_str());
        y += 13;
    }

    u8g2.sendBuffer();
}

int UIDisplay::detailLines(const String& text) {
    return max(1, (utf8Length(text) + COLS - 1) / COLS);
}
