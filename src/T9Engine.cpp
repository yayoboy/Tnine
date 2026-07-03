#include "T9Engine.h"

#include <string.h>

#include "config.h"

// Sequenze multi-tap: prima le lettere, poi la cifra, poi le accentate.
// Ogni voce è una stringa UTF-8 (le accentate occupano 2 byte).
static const char* const SEQ_0[] = {" ", "0"};
static const char* const SEQ_1[] = {".", ",", "?", "!", "'", "\"", "-", "@", "/", ":", "1"};
static const char* const SEQ_2[] = {"a", "b", "c", "2", "à"};
static const char* const SEQ_3[] = {"d", "e", "f", "3", "è", "é"};
static const char* const SEQ_4[] = {"g", "h", "i", "4", "ì"};
static const char* const SEQ_5[] = {"j", "k", "l", "5"};
static const char* const SEQ_6[] = {"m", "n", "o", "6", "ò"};
static const char* const SEQ_7[] = {"p", "q", "r", "s", "7"};
static const char* const SEQ_8[] = {"t", "u", "v", "8", "ù"};
static const char* const SEQ_9[] = {"w", "x", "y", "z", "9"};

struct KeySequence {
    const char* const* items;
    uint8_t count;
};

#define SEQ(arr) { arr, sizeof(arr) / sizeof(arr[0]) }
static const KeySequence SEQUENCES[10] = {
    SEQ(SEQ_0), SEQ(SEQ_1), SEQ(SEQ_2), SEQ(SEQ_3), SEQ(SEQ_4),
    SEQ(SEQ_5), SEQ(SEQ_6), SEQ(SEQ_7), SEQ(SEQ_8), SEQ(SEQ_9),
};
#undef SEQ

static const KeySequence* sequenceFor(char key) {
    if (key < '0' || key > '9') return nullptr;
    return &SEQUENCES[key - '0'];
}

// Maiuscola di un carattere UTF-8: ASCII con toupper; per le accentate
// latine (C3 A0..C3 BE) il secondo byte va abbassato di 0x20 (à -> À).
static String utf8Upper(const char* s) {
    String out;
    size_t len = strlen(s);
    if (len == 1) {
        out += static_cast<char>(toupper(s[0]));
    } else if (len == 2 && static_cast<uint8_t>(s[0]) == 0xC3 &&
               static_cast<uint8_t>(s[1]) >= 0xA0 &&
               static_cast<uint8_t>(s[1]) <= 0xBE) {
        out += s[0];
        out += static_cast<char>(static_cast<uint8_t>(s[1]) - 0x20);
    } else {
        out = s;
    }
    return out;
}

String T9Engine::candidate() const {
    if (!_pendingKey) return String();
    const KeySequence* seq = sequenceFor(_pendingKey);
    const char* c = seq->items[_pendingIndex];
    if (_mode == T9Mode::Upper) return utf8Upper(c);
    return String(c);
}

String T9Engine::lastGlyph() const {
    if (_text.length() == 0) return String();
    // Torna indietro oltre gli eventuali byte di continuazione UTF-8.
    int start = _text.length() - 1;
    while (start > 0 && (static_cast<uint8_t>(_text.charAt(start)) & 0xC0) == 0x80) {
        --start;
    }
    return _text.substring(start, _text.length());
}

bool T9Engine::handleKey(char key, uint32_t now) {
    const KeySequence* seq = sequenceFor(key);
    if (!seq) return false;

    if (_mode == T9Mode::Number) {
        commitPending();
        if (_text.length() < MESSAGE_MAX_LEN) _text += key;
        return true;
    }

    if (key == _pendingKey && now - _lastPress < T9_MULTITAP_MS) {
        _pendingIndex = (_pendingIndex + 1) % seq->count;
    } else {
        commitPending();
        _pendingKey = key;
        _pendingIndex = 0;
    }
    _lastPress = now;
    return true;
}

bool T9Engine::update(uint32_t now) {
    if (_pendingKey && now - _lastPress >= T9_MULTITAP_MS) {
        commitPending();
        return true;
    }
    return false;
}

void T9Engine::commitPending() {
    if (!_pendingKey) return;
    String c = candidate();
    _pendingKey = 0;
    _pendingIndex = 0;
    if (_text.length() + c.length() <= MESSAGE_MAX_LEN) _text += c;
}

bool T9Engine::backspace() {
    if (_pendingKey) {
        // Annulla il candidato non ancora confermato.
        _pendingKey = 0;
        _pendingIndex = 0;
        return true;
    }
    if (_text.length() > 0) {
        // Rimuove un intero carattere UTF-8, non un singolo byte.
        int cut = _text.length() - 1;
        while (cut > 0 && (static_cast<uint8_t>(_text.charAt(cut)) & 0xC0) == 0x80) {
            --cut;
        }
        _text.remove(cut);
        return true;
    }
    return false;
}

void T9Engine::clear() {
    _pendingKey = 0;
    _pendingIndex = 0;
    _text = "";
}

void T9Engine::cycleMode() {
    commitPending();
    switch (_mode) {
        case T9Mode::Lower:  _mode = T9Mode::Upper;  break;
        case T9Mode::Upper:  _mode = T9Mode::Number; break;
        case T9Mode::Number: _mode = T9Mode::Lower;  break;
    }
}

const char* T9Engine::modeLabel() const {
    switch (_mode) {
        case T9Mode::Lower:  return "abc";
        case T9Mode::Upper:  return "ABC";
        case T9Mode::Number: return "123";
    }
    return "?";
}
