#include "PreviewDisplay.h"

#include <U8g2lib.h>
#include <Wire.h>

#include "T9Engine.h"
#include "config.h"

// Il modulo 0.42" usa un SSD1306 con area visibile 72x40: la variante _ER_
// di U8g2 applica l'offset corretto.
static U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0);

void PreviewDisplay::begin() {
    Wire.setSDA(PIN_PREVIEW_SDA);
    Wire.setSCL(PIN_PREVIEW_SCL);
    u8g2.begin();
    u8g2.enableUTF8Print();
}

void PreviewDisplay::render(const T9Engine& t9) {
    u8g2.clearBuffer();

    String glyph = t9.candidate();
    if (glyph.length() == 0) glyph = t9.lastGlyph();

    if (glyph.length() > 0) {
        if (glyph == " ") glyph = "_";  // lo spazio si mostra come '_'

        // Font grande; se il glifo non è coperto (larghezza 0) si ripiega
        // su un font più piccolo ma con l'intero latino esteso.
        u8g2.setFont(u8g2_font_logisoso28_tf);
        int w = u8g2.getUTF8Width(glyph.c_str());
        int baseline = 34;
        if (w == 0) {
            u8g2.setFont(u8g2_font_10x20_te);
            w = u8g2.getUTF8Width(glyph.c_str());
            baseline = 27;
        }
        u8g2.drawUTF8((72 - w) / 2, baseline, glyph.c_str());

        // Modalità in piccolo nell'angolo.
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.drawStr(1, 39, t9.modeLabel());
    } else {
        u8g2.setFont(u8g2_font_9x15_te);
        const char* label = t9.modeLabel();
        int w = u8g2.getStrWidth(label);
        u8g2.drawStr((72 - w) / 2, 26, label);
    }

    u8g2.sendBuffer();
}
