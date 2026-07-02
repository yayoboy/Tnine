#include "T9Engine.h"
#include "config.h"

// Sequenze multi-tap classiche; '1' contiene la punteggiatura.
static const char* const SEQUENCES[10] = {
    " 0",           // 0: spazio, poi cifra
    ".,?!'\"-@/:1", // 1: punteggiatura
    "abc2",
    "def3",
    "ghi4",
    "jkl5",
    "mno6",
    "pqrs7",
    "tuv8",
    "wxyz9",
};

const char* T9Engine::sequenceFor(char key) const {
    if (key < '0' || key > '9') return nullptr;
    return SEQUENCES[key - '0'];
}

char T9Engine::candidate() const {
    if (!_pendingKey) return '\0';
    const char* seq = sequenceFor(_pendingKey);
    char c = seq[_pendingIndex];
    if (_mode == T9Mode::Upper) c = toupper(c);
    return c;
}

bool T9Engine::handleKey(char key, uint32_t now) {
    const char* seq = sequenceFor(key);
    if (!seq) return false;

    if (_mode == T9Mode::Number) {
        commitPending();
        if (_text.length() < MESSAGE_MAX_LEN) _text += key;
        return true;
    }

    if (key == _pendingKey && now - _lastPress < T9_MULTITAP_MS) {
        _pendingIndex = (_pendingIndex + 1) % strlen(seq);
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
    char c = candidate();
    _pendingKey = 0;
    _pendingIndex = 0;
    if (_text.length() < MESSAGE_MAX_LEN) _text += c;
}

bool T9Engine::backspace() {
    if (_pendingKey) {
        // Annulla il candidato non ancora confermato.
        _pendingKey = 0;
        _pendingIndex = 0;
        return true;
    }
    if (_text.length() > 0) {
        _text.remove(_text.length() - 1);
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
