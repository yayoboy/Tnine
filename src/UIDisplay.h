#pragma once

#include <Arduino.h>

class T9Engine;

// OLED 128x64 (SSD1306) su I2C1. Schermate:
//  - composizione: barra di stato, testo con cursore, ultimo messaggio
//  - lista: titolo + voci scorrevoli con selezione (menu, nodi, canali...)
//  - dettaglio: titolo + testo lungo con scorrimento verticale
class UIDisplay {
public:
    void begin();

    void renderCompose(const T9Engine& t9, const String& lastRx,
                       const String& statusRight);
    void renderList(const char* title, const String* items, int count,
                    int selected);
    void renderDetail(const char* title, const String& text, int scrollLine);

    // Righe di testo necessarie per una stringa nella vista dettaglio.
    static int detailLines(const String& text);
};
