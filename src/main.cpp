// Tnine – tastiera T9 per Meshtastic su RP2040
//
// Comandi:
//   0-9        input T9 multi-tap (in modalità 123 inserisce la cifra)
//   * breve    backspace (o annulla il carattere candidato)
//   * lungo    cancella tutto il messaggio
//   # breve    invia il messaggio al nodo Meshtastic
//   # lungo    cambia modalità (abc -> ABC -> 123)

#include <Arduino.h>

#include "MatrixKeypad.h"
#include "MeshtasticLink.h"
#include "PreviewDisplay.h"
#include "T9Engine.h"
#include "UIDisplay.h"
#include "config.h"

static MatrixKeypad keypad;
static T9Engine t9;
static MeshtasticLink mesh;
static PreviewDisplay preview;
static UIDisplay ui;

static String lastRx;
static String status;
static uint32_t statusUntil = 0;
static bool dirty = true;

static void setStatus(const char* msg, uint32_t now, uint32_t durationMs = 2000) {
    status = msg;
    statusUntil = now + durationMs;
    dirty = true;
}

static void onMeshRx(const String& line) {
    lastRx = line;
    dirty = true;
}

static void sendMessage(uint32_t now) {
    t9.commitPending();
    if (t9.text().length() == 0) {
        setStatus("vuoto", now);
        return;
    }
    if (mesh.sendText(t9.text())) {
        t9.clear();
        setStatus("inviato", now);
    } else {
        setStatus("errore", now);
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
            setStatus("cancellato", now);
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
    mesh.begin(onMeshRx);
    preview.begin();
    ui.begin();
    setStatus("pronto", millis());
}

void loop() {
    uint32_t now = millis();

    mesh.poll();

    KeyEvent ev = keypad.poll(now);
    if (ev.type != KeyEvent::None) {
        handleKeyEvent(ev, now);
    }

    dirty |= t9.update(now);

    if (status.length() > 0 && (int32_t)(now - statusUntil) >= 0) {
        status = "";
        dirty = true;
    }

    if (dirty) {
        dirty = false;
        preview.render(t9);
        ui.render(t9, lastRx, status);
    }
}
