#pragma once

#include <Arduino.h>

class T9Engine;

// OLED 128x64 (SSD1306) su I2C1: barra di stato (modalità, byte usati,
// stato del collegamento), area di composizione del messaggio e ultimo
// messaggio ricevuto dalla rete mesh.
class UIDisplay {
public:
    void begin();
    void render(const T9Engine& t9, const String& lastRx,
                const String& status, const String& linkLabel);
};
