#pragma once

#include <Arduino.h>

enum class T9Mode : uint8_t { Lower, Upper, Number };

// Input testo T9 multi-tap in stile telefonico:
//  - premere ripetutamente lo stesso tasto entro T9_MULTITAP_MS scorre i
//    caratteri associati; il carattere corrente ("candidato") viene
//    confermato al timeout o alla pressione di un tasto diverso.
//  - in modalità Number ogni tasto inserisce direttamente la cifra.
class T9Engine {
public:
    // Gestisce un tasto '0'..'9'. Ritorna true se lo stato è cambiato.
    bool handleKey(char key, uint32_t now);

    // Conferma il candidato al timeout. Ritorna true se lo stato è cambiato.
    bool update(uint32_t now);

    void commitPending();
    bool backspace();
    void clear();
    void cycleMode();

    bool hasPending() const { return _pendingKey != 0; }
    char candidate() const;         // carattere candidato ('\0' se nessuno)
    T9Mode mode() const { return _mode; }
    const char* modeLabel() const;
    const String& text() const { return _text; }

private:
    const char* sequenceFor(char key) const;

    String _text;
    T9Mode _mode = T9Mode::Lower;
    char _pendingKey = 0;
    uint8_t _pendingIndex = 0;
    uint32_t _lastPress = 0;
};
