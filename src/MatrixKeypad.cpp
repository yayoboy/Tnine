#include "MatrixKeypad.h"
#include "config.h"

static const char LAYOUT[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'},
};

void MatrixKeypad::begin() {
    for (uint8_t c = 0; c < KEYPAD_COLS; ++c) {
        pinMode(KEYPAD_COL_PINS[c], INPUT_PULLUP);
    }
    // Le righe restano in alta impedenza finché non vengono scandite.
    for (uint8_t r = 0; r < KEYPAD_ROWS; ++r) {
        pinMode(KEYPAD_ROW_PINS[r], INPUT);
    }
}

char MatrixKeypad::scan() {
    char found = 0;
    for (uint8_t r = 0; r < KEYPAD_ROWS && !found; ++r) {
        pinMode(KEYPAD_ROW_PINS[r], OUTPUT);
        digitalWrite(KEYPAD_ROW_PINS[r], LOW);
        delayMicroseconds(5);
        for (uint8_t c = 0; c < KEYPAD_COLS; ++c) {
            if (digitalRead(KEYPAD_COL_PINS[c]) == LOW) {
                found = LAYOUT[r][c];
                break;
            }
        }
        pinMode(KEYPAD_ROW_PINS[r], INPUT);
    }
    return found;
}

KeyEvent MatrixKeypad::poll(uint32_t now) {
    KeyEvent ev;

    char raw = scan();
    if (raw != _raw) {
        _raw = raw;
        _rawSince = now;
        return ev;
    }
    if (now - _rawSince < KEY_DEBOUNCE_MS) {
        return ev;
    }

    char stable = _raw;

    if (_current == 0 && stable != 0) {
        _current = stable;
        _downAt = now;
        _longFired = false;
        ev.type = KeyEvent::Down;
        ev.key = _current;
        return ev;
    }

    if (_current != 0 && stable != _current) {
        // Rilascio (o passaggio diretto a un altro tasto: il nuovo tasto
        // verrà rilevato come Down al prossimo poll).
        ev.type = KeyEvent::Up;
        ev.key = _current;
        ev.wasLong = _longFired;
        _current = 0;
        return ev;
    }

    if (_current != 0 && !_longFired && now - _downAt >= KEY_LONGPRESS_MS) {
        _longFired = true;
        ev.type = KeyEvent::LongHold;
        ev.key = _current;
        return ev;
    }

    return ev;
}
