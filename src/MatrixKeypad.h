#pragma once

#include <Arduino.h>

// Evento prodotto dal tastierino.
//  - Down:     tasto appena premuto (debounced)
//  - LongHold: il tasto è tenuto premuto oltre KEY_LONGPRESS_MS (emesso una volta)
//  - Up:       tasto rilasciato; wasLong indica se LongHold era già scattato
struct KeyEvent {
    enum Type : uint8_t { None, Down, LongHold, Up };
    Type type = None;
    char key = 0;
    bool wasLong = false;
};

// Scansione di un tastierino a matrice 4x3 con antirimbalzo e rilevamento
// della pressione lunga. Gestisce un solo tasto alla volta: per il T9 non
// serve il rollover.
class MatrixKeypad {
public:
    void begin();
    KeyEvent poll(uint32_t now);

private:
    char scan();

    char _raw = 0;          // ultima lettura grezza
    uint32_t _rawSince = 0; // da quando la lettura grezza è stabile
    char _current = 0;      // tasto attualmente premuto (debounced), 0 = nessuno
    uint32_t _downAt = 0;   // istante della pressione
    bool _longFired = false;
};
