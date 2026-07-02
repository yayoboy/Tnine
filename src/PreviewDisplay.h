#pragma once

#include <Arduino.h>

class T9Engine;

// OLED 0.42" (SSD1306 72x40) su I2C0: mostra in grande il carattere
// candidato durante il multi-tap, oppure la modalità corrente quando non
// c'è nessun candidato attivo.
class PreviewDisplay {
public:
    void begin();
    void render(const T9Engine& t9);
};
