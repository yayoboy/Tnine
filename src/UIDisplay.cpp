#include "UIDisplay.h"

#include <U8g2lib.h>
#include <Wire.h>

#include "RetroUI.h"
#include "T9Engine.h"
#include "config.h"

// _2ND_HW_I2C usa Wire1 sul core arduino-pico.
static U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C u8g2(U8G2_R0);

// Corpo testo 6x12 dentro la finestra -> 20 colonne utili.
static constexpr int BODY_COLS = 20;
static constexpr int LIST_ROWS = 4;

using namespace retroui;

// --- Utilità UTF-8 -----------------------------------------------------------

static bool isUtf8Start(char c) {
    return (static_cast<uint8_t>(c) & 0xC0) != 0x80;
}

static int utf8Length(const String& s) {
    int n = 0;
    for (unsigned i = 0; i < s.length(); ++i) {
        if (isUtf8Start(s.charAt(i))) ++n;
    }
    return n;
}

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

static String truncated(const String& s, int maxGlyphs) {
    if (utf8Length(s) <= maxGlyphs) return s;
    return s.substring(0, utf8ByteIndex(s, maxGlyphs - 1)) + ">";
}

// Disegna il corpo testo (font 6x12, accenti supportati) a partire dalla
// riga `firstLine`, per `rows` righe, dentro la finestra.
static void drawBody(const String& text, int firstLine, int rows) {
    u8g2.setFont(u8g2_font_6x12_te);
    int total = (utf8Length(text) + BODY_COLS - 1) / BODY_COLS;
    int y = 13;
    for (int line = firstLine; line < total && line < firstLine + rows; ++line) {
        unsigned from = utf8ByteIndex(text, line * BODY_COLS);
        unsigned to = utf8ByteIndex(text, (line + 1) * BODY_COLS);
        u8g2.drawUTF8(3, y, text.substring(from, to).c_str());
        y += 12;
    }
}

void UIDisplay::begin() {
    Wire1.setSDA(PIN_UI_SDA);
    Wire1.setSCL(PIN_UI_SCL);
    u8g2.begin();
    u8g2.enableUTF8Print();
    // Coordinate testo "top": i valori del prototipo si trasferiscono 1:1.
    u8g2.setFontPosTop();
}

// --- Boot ----------------------------------------------------------------------

void UIDisplay::renderBoot(int progressPct) {
    u8g2.clearBuffer();
    win(u8g2, 0, 0, 128, 64, "TNINE", "T9 MESH");
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(64 - u8g2.getStrWidth("TNINE") / 2, 16, "TNINE");
    gauge(u8g2, 8, 42, 84, progressPct);
    text5x7R(u8g2, 124, 41, String(progressPct) + "%");
    text5x7(u8g2, 8, 53, "CONNESSIONE MESH...");
    u8g2.sendBuffer();
}

// --- Composizione ----------------------------------------------------------------

void UIDisplay::renderCompose(const T9Engine& t9, const String& titleRight,
                              int battPct, uint8_t unread) {
    u8g2.clearBuffer();
    win(u8g2, 0, 0, 128, 64, "COMPONI", titleRight);

    // Corpo: coda del testo + candidato + cursore, 3 righe.
    String compose = t9.text();
    compose += t9.candidate();
    compose += '_';
    int total = (utf8Length(compose) + BODY_COLS - 1) / BODY_COLS;
    drawBody(compose, max(0, total - 3), 3);

    // Footer: modalità, contatore, nuovi messaggi, batteria.
    u8g2.drawHLine(1, 50, 126);
    text5x7(u8g2, 3, 54, t9.modeLabel());
    char cnt[16];
    snprintf(cnt, sizeof(cnt), "%u/%u",
             static_cast<unsigned>(t9.text().length()),
             static_cast<unsigned>(MESSAGE_MAX_LEN));
    text5x7(u8g2, 24, 54, cnt);

    if (unread > 0) {
        envelope(u8g2, 62, 54);
        text5x7(u8g2, 74, 54, String(unread));
    }

    if (battPct > 100) {
        text5x7R(u8g2, 124, 54, "USB");
    } else if (battPct >= 0) {
        gauge(u8g2, 94, 55, 30, battPct);
    }

    u8g2.sendBuffer();
}

// --- Liste -----------------------------------------------------------------------

void UIDisplay::renderList(const char* title, const String& titleRight,
                           const String* items, int count, int selected,
                           const int8_t* bars) {
    u8g2.clearBuffer();
    win(u8g2, 0, 0, 128, 64, title, titleRight);

    bool scrollbar = count > LIST_ROWS;
    int rowW = scrollbar ? 114 : 124;
    int textCols = (bars ? 14 : rowW / 5 - 1);

    int first = 0;
    if (selected >= LIST_ROWS) first = selected - LIST_ROWS + 1;
    if (first > count - LIST_ROWS) first = max(0, count - LIST_ROWS);

    for (int i = first; i < count && i < first + LIST_ROWS; ++i) {
        int y = 13 + (i - first) * 12;
        bool sel = (i == selected);
        if (sel) u8g2.drawBox(2, y - 1, rowW, 11);
        if (sel) u8g2.setDrawColor(0);
        text5x7(u8g2, 4, y + 1, truncated(items[i], textCols));
        if (bars && bars[i] >= 0) {
            retroui::bars(u8g2, rowW - 12, y + 8, bars[i]);
        }
        u8g2.setDrawColor(1);
    }

    if (scrollbar) vscroll(u8g2, 119, 12, 50, selected, count, LIST_ROWS);

    u8g2.sendBuffer();
}

// --- Dettaglio ----------------------------------------------------------------------

void UIDisplay::renderDetail(const char* title, const String& titleRight,
                             const String& text, int scrollLine,
                             const char* btnA, const char* btnB, int selBtn) {
    u8g2.clearBuffer();
    win(u8g2, 0, 0, 128, 64, title, titleRight);

    bool buttons = (btnA != nullptr);
    int rows = buttons ? 3 : 4;
    drawBody(text, scrollLine, rows);

    int total = detailLines(text);
    if (total > rows) {
        vscroll(u8g2, 119, 12, buttons ? 35 : 50, scrollLine, total, rows);
    }

    if (buttons) {
        button(u8g2, 3, 49, 58, 13, btnA, selBtn == 0);
        if (btnB) button(u8g2, 66, 49, 46, 13, btnB, selBtn == 1);
    }

    u8g2.sendBuffer();
}

int UIDisplay::detailLines(const String& text) {
    return max(1, (utf8Length(text) + BODY_COLS - 1) / BODY_COLS);
}

// --- Info -------------------------------------------------------------------------

void UIDisplay::renderInfo(const String& nodeId, int battPct, bool linkOk,
                           int nodeCount, const String& dest, const String& chan) {
    u8g2.clearBuffer();
    win(u8g2, 0, 0, 128, 64, "INFO", nodeId);

    text5x7(u8g2, 4, 14, "BATT");
    if (battPct > 100) {
        text5x7(u8g2, 34, 14, "ALIMENTATO USB");
    } else if (battPct >= 0) {
        gauge(u8g2, 34, 15, 60, battPct);
        text5x7R(u8g2, 124, 14, String(battPct) + "%");
    } else {
        text5x7(u8g2, 34, 14, "?");
    }

    text5x7(u8g2, 4, 26, String("NODI ") + String(nodeCount));
    text5x7R(u8g2, 124, 26, linkOk ? "MESH OK" : "ATTESA...");

    dotted(u8g2, 4, 36, 120);

    text5x7(u8g2, 4, 41, String("DEST   ") + dest);
    text5x7(u8g2, 4, 51, String("CANALE ") + chan);

    u8g2.sendBuffer();
}
