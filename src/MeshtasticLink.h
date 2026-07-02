#pragma once

#include <Arduino.h>

// Collegamento al modulo seriale di Meshtastic in modalità TEXTMSG:
// ogni riga inviata sulla UART viene trasmessa dal nodo come messaggio di
// testo sul canale primario; i messaggi ricevuti arrivano come righe di testo.
class MeshtasticLink {
public:
    using RxCallback = void (*)(const String& line);

    void begin(RxCallback onRx);
    void poll();
    bool sendText(const String& msg);

private:
    RxCallback _onRx = nullptr;
    String _rxLine;
};
