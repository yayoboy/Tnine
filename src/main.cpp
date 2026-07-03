// Tnine – tastiera T9 per Meshtastic su RP2040
//
// Comandi:
//   0-9        input T9 multi-tap (in modalità 123 inserisce la cifra)
//   * breve    backspace (o annulla il carattere candidato)
//   * lungo    cancella tutto il messaggio
//   # breve    invia il messaggio alla rete mesh (broadcast, con ACK)
//   # lungo    cambia modalità (abc -> ABC -> 123)

#include <Arduino.h>

#include "MatrixKeypad.h"
#include "MeshtasticClient.h"
#include "PreviewDisplay.h"
#include "T9Engine.h"
#include "UIDisplay.h"
#include "config.h"

static MatrixKeypad keypad;
static T9Engine t9;
static MeshtasticClient mesh;
static PreviewDisplay preview;
static UIDisplay ui;

static String lastRx;
static String status;
static uint32_t statusUntil = 0;
static String linkLabel = "mesh...";
static uint32_t pendingPacketId = 0;
static bool dirty = true;

static void setStatus(const char* msg, uint32_t durationMs = 3000) {
    status = msg;
    statusUntil = millis() + durationMs;
    dirty = true;
}

// --- Callback dal client Meshtastic -----------------------------------------

static void onTextMessage(uint32_t fromNode, const char* senderName, const String& text) {
    if (senderName && senderName[0] != '\0') {
        lastRx = String(senderName) + ": " + text;
    } else {
        char hex[12];
        snprintf(hex, sizeof(hex), "%08lx", static_cast<unsigned long>(fromNode));
        lastRx = String(hex + 4) + ": " + text;  // ultime 4 cifre dell'id nodo
    }
    dirty = true;
}

static void onSendResult(uint32_t packetId, uint32_t error) {
    if (packetId != pendingPacketId) return;
    pendingPacketId = 0;
    if (error == meshproto::ROUTING_ERROR_NONE) {
        setStatus("consegnato");
    } else {
        setStatus(meshproto::routingErrorLabel(error));
    }
}

static void onLinkStateChange(MeshtasticClient::LinkState state) {
    linkLabel = (state == MeshtasticClient::LinkState::Connected) ? "mesh ok" : "mesh...";
    dirty = true;
}

// --- Gestione tasti ----------------------------------------------------------

static void sendMessage(uint32_t now) {
    (void)now;
    t9.commitPending();
    if (t9.text().length() == 0) {
        setStatus("vuoto");
        return;
    }
    uint32_t id = mesh.sendText(t9.text());
    if (id != 0) {
        pendingPacketId = id;
        t9.clear();
        setStatus("invio...", 30000);  // sostituito dall'esito dell'ACK
    } else {
        setStatus("errore");
    }
}

static void handleKeyEvent(const KeyEvent& ev, uint32_t now) {
    if (ev.key >= '0' && ev.key <= '9') {
        if (ev.type == KeyEvent::Down) {
            dirty |= t9.handleKey(ev.key, now);
        }
        return;
    }

    if (ev.key == '*') {
        if (ev.type == KeyEvent::Up && !ev.wasLong) {
            dirty |= t9.backspace();
        } else if (ev.type == KeyEvent::LongHold) {
            t9.clear();
            setStatus("cancellato");
        }
        return;
    }

    if (ev.key == '#') {
        if (ev.type == KeyEvent::Up && !ev.wasLong) {
            sendMessage(now);
        } else if (ev.type == KeyEvent::LongHold) {
            t9.cycleMode();
            dirty = true;
        }
        return;
    }
}

void setup() {
    Serial.begin(115200);  // log di debug via USB

    keypad.begin();

    MeshtasticClient::Callbacks cb;
    cb.onTextMessage = onTextMessage;
    cb.onSendResult = onSendResult;
    cb.onLinkStateChange = onLinkStateChange;
    mesh.begin(cb);

    preview.begin();
    ui.begin();
}

void loop() {
    uint32_t now = millis();

    mesh.poll(now);

    KeyEvent ev = keypad.poll(now);
    if (ev.type != KeyEvent::None) {
        handleKeyEvent(ev, now);
    }

    dirty |= t9.update(now);

    if (status.length() > 0 && static_cast<int32_t>(now - statusUntil) >= 0) {
        status = "";
        dirty = true;
    }

    if (dirty) {
        dirty = false;
        preview.render(t9);
        ui.render(t9, lastRx, status, linkLabel);
    }
}
