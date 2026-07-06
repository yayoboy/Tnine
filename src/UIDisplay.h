#pragma once

#include <Arduino.h>

class T9Engine;

// OLED 128x64 (SSD1306) su I2C1, con la UI "Retro '84" del prototipo:
// finestre con barra titolo a righe, liste con scrollbar tratteggiata,
// gauge, pulsanti. Schermate:
//  - boot: splash con gauge di connessione
//  - composizione: editor T9 con footer (modalità, contatore, batteria)
//  - lista: voci scorrevoli con selezione invertita e barre di segnale
//  - dettaglio: testo scorrevole con pulsanti opzionali
//  - info: stato del nodo con gauge batteria
class UIDisplay {
public:
    void begin();

    void renderBoot(int progressPct);
    void renderCompose(const T9Engine& t9, const String& titleRight,
                       int battPct, uint8_t unread);
    // bars: array parallelo a items con 0..4 tacche di segnale (o nullptr).
    void renderList(const char* title, const String& titleRight,
                    const String* items, int count, int selected,
                    const int8_t* bars = nullptr);
    // btnA/btnB: pulsanti in basso (nullptr = nessuno); selBtn 0/1.
    void renderDetail(const char* title, const String& titleRight,
                      const String& text, int scrollLine,
                      const char* btnA = nullptr, const char* btnB = nullptr,
                      int selBtn = 0);
    void renderInfo(const String& nodeId, int battPct, bool linkOk,
                    int nodeCount, const String& dest, const String& chan);

    // Righe di testo necessarie per una stringa nella vista dettaglio.
    static int detailLines(const String& text);
};
