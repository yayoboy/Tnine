// Test nativi della logica T9 (multi-tap, UTF-8, modalità, limiti).
#include "T9Engine.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "config.h"

int main() {
    T9Engine t9;
    uint32_t now = 0;

    // "ciao": c = 2x3, i = 4x3, a = 2x1, o = 6x3
    for (int i = 0; i < 3; ++i) t9.handleKey('2', now += 100);
    assert(t9.candidate() == "c");
    t9.update(now += 1000);              // conferma al timeout
    for (int i = 0; i < 3; ++i) t9.handleKey('4', now += 100);
    t9.handleKey('2', now += 1000);      // tasto diverso conferma la 'i'
    assert(t9.candidate() == "a");
    for (int i = 0; i < 3; ++i) t9.handleKey('6', now += 100);
    t9.commitPending();
    assert(t9.text() == "ciao");

    // Spazio con '0'
    t9.handleKey('0', now += 1000);
    t9.commitPending();
    assert(t9.text() == "ciao ");

    // Accentate: quinta pressione su '2' -> 'à'
    for (int i = 0; i < 5; ++i) t9.handleKey('2', now += 100);
    assert(t9.candidate() == "à");
    t9.commitPending();
    assert(t9.text() == "ciao à");
    assert(t9.lastGlyph() == "à");

    // Il backspace rimuove l'intero carattere UTF-8 (2 byte)
    assert(t9.backspace());
    assert(t9.text() == "ciao ");

    // Maiuscole, anche accentate
    t9.cycleMode();
    assert(strcmp(t9.modeLabel(), "ABC") == 0);
    t9.handleKey('2', now += 1000);
    assert(t9.candidate() == "A");
    for (int i = 0; i < 4; ++i) t9.handleKey('2', now += 100);
    assert(t9.candidate() == "À");
    t9.commitPending();
    assert(t9.text() == "ciao À");

    // Modalità numerica: cifra diretta, nessun candidato
    t9.cycleMode();
    assert(strcmp(t9.modeLabel(), "123") == 0);
    t9.handleKey('5', now += 100);
    assert(t9.text() == "ciao À5");
    assert(!t9.hasPending());

    // Il backspace annulla prima il candidato
    t9.cycleMode();  // torna a minuscole
    t9.handleKey('3', now += 1000);
    assert(t9.hasPending());
    assert(t9.backspace());
    assert(!t9.hasPending());
    assert(t9.text() == "ciao À5");

    // Il multi-tap torna all'inizio: 6 pressioni su '2' (a b c 2 à -> a)
    t9.clear();
    for (int i = 0; i < 6; ++i) t9.handleKey('2', now += 100);
    assert(t9.candidate() == "a");
    t9.clear();
    assert(t9.text().length() == 0);

    // Punteggiatura su '1'
    t9.handleKey('1', now += 1000);
    assert(t9.candidate() == ".");
    t9.handleKey('1', now += 100);
    assert(t9.candidate() == ",");
    t9.clear();

    // setText (messaggi rapidi): sostituisce il testo e tronca in modo
    // UTF-8-safe
    t9.setText("Posizione?");
    assert(t9.text() == "Posizione?");
    {
        String longMsg;
        for (size_t i = 0; i < MESSAGE_MAX_LEN + 3; ++i) longMsg += "à";  // 2 byte l'uno
        t9.setText(longMsg);
        assert(t9.text().length() <= MESSAGE_MAX_LEN);
        assert(t9.text().length() % 2 == 0);  // nessuna 'à' spezzata
    }
    t9.clear();

    // Limite in byte: mai oltre MESSAGE_MAX_LEN
    for (size_t i = 0; i < MESSAGE_MAX_LEN + 50; ++i) {
        t9.handleKey('2', now += 1000);
        t9.update(now += 1000);
    }
    assert(t9.text().length() == MESSAGE_MAX_LEN);

    // ... anche con caratteri da 2 byte
    t9.clear();
    for (size_t i = 0; i < MESSAGE_MAX_LEN; ++i) {
        for (int p = 0; p < 5; ++p) t9.handleKey('2', now += 100);  // 'à'
        t9.update(now += 1000);
    }
    assert(t9.text().length() <= MESSAGE_MAX_LEN);

    printf("test_t9: tutti i test superati.\n");
    return 0;
}
