#pragma once

#include <Arduino.h>

enum class T9Mode : uint8_t { Lower, Upper, Number };

// Input testo T9 multi-tap in stile telefonico:
//  - premere ripetutamente lo stesso tasto entro T9_MULTITAP_MS scorre i
//    caratteri associati; il carattere corrente ("candidato") viene
//    confermato al timeout o alla pressione di un tasto diverso.
//  - in modalità Number ogni tasto inserisce direttamente la cifra.
//
// I caratteri sono stringhe UTF-8 (una lettera può occupare più byte: à, è…).
// Il limite MESSAGE_MAX_LEN è in byte, come il payload di Meshtastic.
class T9Engine {
public:
    // Gestisce un tasto '0'..'9'. Ritorna true se lo stato è cambiato.
    bool handleKey(char key, uint32_t now);

    // Conferma il candidato al timeout. Ritorna true se lo stato è cambiato.
    bool update(uint32_t now);

    void commitPending();
    bool backspace();   // annulla il candidato, oppure cancella l'ultimo carattere
    void clear();
    void cycleMode();
    // Sostituisce il testo (es. messaggio rapido), troncato a MESSAGE_MAX_LEN.
    void setText(const String& s);

    bool hasPending() const { return _pendingKey != 0; }
    String candidate() const;   // carattere candidato UTF-8 ("" se nessuno)
    String lastGlyph() const;   // ultimo carattere confermato ("" se vuoto)
    T9Mode mode() const { return _mode; }
    const char* modeLabel() const;
    const String& text() const { return _text; }

private:
    String _text;
    T9Mode _mode = T9Mode::Lower;
    char _pendingKey = 0;
    uint8_t _pendingIndex = 0;
    uint32_t _lastPress = 0;
};
