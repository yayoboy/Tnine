#pragma once

#include <U8g2lib.h>

// Kit di widget bitmap in stile "Retro '84" per OLED 128x64, trascritto dal
// prototipo UI (oled.js): finestre con barra titolo a righe, scrollbar
// tratteggiate, gauge con pomello, pulsanti arrotondati, barre di segnale,
// step-tab a chevron.
//
// Convenzione: le coordinate testo sono "top" (chiamare setFontPosTop() in
// begin), così i valori del prototipo si trasferiscono 1:1.
namespace retroui {

// Testo 5x7 con traslitterazione delle accentate (à -> a), come nel
// prototipo: il font di sistema retro non le copre.
void text5x7(U8G2& d, int x, int y, const String& s);
void text5x7R(U8G2& d, int xRight, int y, const String& s);   // allineato a destra
void text5x7C(U8G2& d, int xCenter, int y, const String& s);  // centrato
int width5x7(U8G2& d, const String& s);

// Riempimento a tratteggio (scacchiera 1px).
void hatch(U8G2& d, int x, int y, int w, int h);

// Linea orizzontale punteggiata (1 px ogni 3).
void dotted(U8G2& d, int x, int y, int w);

// Finestra con cornice, barra titolo con righe decorative e testo opzionale
// a destra. Occupa la fascia y..y+10 per la barra.
void win(U8G2& d, int x, int y, int w, int h, const String& title,
         const String& rtext = String());

// Gauge orizzontale (altezza 5) con riempimento a tratteggio e pomello.
void gauge(U8G2& d, int x, int y, int w, int pct);

// Scrollbar verticale (larghezza 7) con thumb tratteggiato.
void vscroll(U8G2& d, int x, int y, int h, int pos, int total, int visible);

// Pulsante con angoli arrotondati; selezionato = riempito con testo invertito.
void button(U8G2& d, int x, int y, int w, int h, const String& label, bool selected);

// Barre di segnale 0..4.
void bars(U8G2& d, int x, int yBottom, int n);

// Step-tab a chevron con due segmenti (sinistro attivo riempito).
void steps(U8G2& d, int x, int y, int w, const String& a, const String& b);

// Icona busta 9x7 (nuovi messaggi).
void envelope(U8G2& d, int x, int y);

}  // namespace retroui
