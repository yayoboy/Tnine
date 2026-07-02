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
}

void PreviewDisplay::render(const T9Engine& t9) {
    u8g2.clearBuffer();

    char c = t9.candidate();
    if (c == '\0' && t9.text().length() > 0) {
        c = t9.text().charAt(t9.text().length() - 1);
    }

    if (c != '\0') {
        char shown = (c == ' ') ? '_' : c;  // lo spazio si mostra come '_'
        char buf[2] = {shown, '\0'};
        u8g2.setFont(u8g2_font_logisoso28_tf);
        int w = u8g2.getStrWidth(buf);
        u8g2.drawStr((72 - w) / 2, 34, buf);
    } else {
        u8g2.setFont(u8g2_font_9x15_tf);
        const char* label = t9.modeLabel();
        int w = u8g2.getStrWidth(label);
        u8g2.drawStr((72 - w) / 2, 26, label);
    }

    // Modalità in piccolo nell'angolo quando c'è un carattere a schermo.
    if (c != '\0') {
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.drawStr(1, 39, t9.modeLabel());
    }

    u8g2.sendBuffer();
}
