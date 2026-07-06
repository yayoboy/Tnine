#include "RetroUI.h"

namespace retroui {

// Traslittera le accentate italiane (UTF-8 C3xx) in ASCII per il font 5x7.
static String translit(const String& s) {
    String out;
    out.reserve(s.length());
    for (unsigned i = 0; i < s.length(); ++i) {
        uint8_t c = static_cast<uint8_t>(s.charAt(i));
        if (c == 0xC3 && i + 1 < s.length()) {
            uint8_t d = static_cast<uint8_t>(s.charAt(i + 1));
            char base = 0;
            switch (d) {
                case 0xA0: base = 'a'; break;  // à
                case 0xA8: case 0xA9: base = 'e'; break;  // è é
                case 0xAC: base = 'i'; break;  // ì
                case 0xB2: base = 'o'; break;  // ò
                case 0xB9: base = 'u'; break;  // ù
                case 0x80: base = 'A'; break;  // À
                case 0x88: case 0x89: base = 'E'; break;  // È É
                case 0x8C: base = 'I'; break;  // Ì
                case 0x92: base = 'O'; break;  // Ò
                case 0x99: base = 'U'; break;  // Ù
            }
            if (base) {
                out += base;
                ++i;
                continue;
            }
        }
        if (c < 0x80) out += static_cast<char>(c);
        else if ((c & 0xC0) != 0x80) out += '?';  // altro carattere multi-byte
    }
    return out;
}

static void useFont(U8G2& d) { d.setFont(u8g2_font_5x7_tf); }

void text5x7(U8G2& d, int x, int y, const String& s) {
    useFont(d);
    d.drawStr(x, y, translit(s).c_str());
}

int width5x7(U8G2& d, const String& s) {
    useFont(d);
    return d.getStrWidth(translit(s).c_str());
}

void text5x7R(U8G2& d, int xRight, int y, const String& s) {
    text5x7(d, xRight - width5x7(d, s), y, s);
}

void text5x7C(U8G2& d, int xCenter, int y, const String& s) {
    text5x7(d, xCenter - width5x7(d, s) / 2, y, s);
}

void hatch(U8G2& d, int x, int y, int w, int h) {
    for (int j = 0; j < h; ++j)
        for (int i = (j & 1); i < w; i += 2)
            d.drawPixel(x + i, y + j);
}

void dotted(U8G2& d, int x, int y, int w) {
    for (int i = 0; i < w; i += 3) d.drawPixel(x + i, y);
}

void win(U8G2& d, int x, int y, int w, int h, const String& title,
         const String& rtext) {
    d.drawFrame(x, y, w, h);
    d.drawHLine(x, y + 10, w);
    text5x7(d, x + 3, y + 2, title);

    // Righe decorative tra il titolo e il testo a destra.
    int sx = x + 3 + width5x7(d, title) + 6;
    int ex = x + w - 4;
    if (rtext.length() > 0) {
        text5x7R(d, x + w - 4, y + 2, rtext);
        ex = x + w - 4 - width5x7(d, rtext) - 6;
    }
    if (ex > sx) {
        for (int j = 0; j < 4; ++j) d.drawHLine(sx, y + 2 + j * 2, ex - sx + 1);
    }
}

void gauge(U8G2& d, int x, int y, int w, int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    d.drawFrame(x, y, w, 5);
    int fw = ((w - 2) * pct) / 100;
    hatch(d, x + 1, y + 1, fw, 3);
    // Pomello a rombo sopra il bordo del riempimento.
    int cx = x + 1 + fw;
    d.setDrawColor(0);
    d.drawBox(cx - 2, y, 5, 5);
    d.setDrawColor(1);
    d.drawPixel(cx, y);
    d.drawHLine(cx - 1, y + 1, 3);
    d.drawHLine(cx - 2, y + 2, 5);
    d.drawHLine(cx - 1, y + 3, 3);
    d.drawPixel(cx, y + 4);
}

void vscroll(U8G2& d, int x, int y, int h, int pos, int total, int visible) {
    d.drawFrame(x, y, 7, h);
    if (total < 1) total = 1;
    int th = max(6, ((h - 2) * visible) / total);
    int range = max(1, total - visible);
    int ty = y + 1 + ((h - 2 - th) * min(pos, range)) / range;
    hatch(d, x + 1, ty, 5, th);
}

void button(U8G2& d, int x, int y, int w, int h, const String& label, bool selected) {
    d.drawRFrame(x, y, w, h, 1);
    if (selected) {
        d.drawBox(x + 1, y + 1, w - 2, h - 2);
        d.setDrawColor(0);
        text5x7C(d, x + w / 2, y + 3, label);
        d.setDrawColor(1);
    } else {
        text5x7C(d, x + w / 2, y + 3, label);
    }
}

void bars(U8G2& d, int x, int yBottom, int n) {
    for (int i = 0; i < 4; ++i) {
        int h = 2 + i;
        int bx = x + i * 3;
        if (i < n) {
            d.drawBox(bx, yBottom - h + 1, 2, h);
        } else {
            d.drawPixel(bx, yBottom);
            d.drawPixel(bx + 1, yBottom);
        }
    }
}

void steps(U8G2& d, int x, int y, int w, const String& a, const String& b) {
    d.drawFrame(x, y, w, 13);
    int mid = x + w / 2;
    // Segmento sinistro riempito con punta a chevron.
    for (int j = 0; j < 11; ++j) {
        int ext = j < 6 ? j : 10 - j;
        d.drawHLine(x + 1, y + 1 + j, (mid - x - 6) + ext);
    }
    d.setDrawColor(0);
    text5x7C(d, x + (mid - x) / 2 - 2, y + 3, a);
    d.setDrawColor(1);
    text5x7C(d, mid + (x + w - mid) / 2 + 2, y + 3, b);
}

void envelope(U8G2& d, int x, int y) {
    d.drawFrame(x, y, 9, 7);
    d.drawLine(x, y, x + 4, y + 3);
    d.drawLine(x + 8, y, x + 4, y + 3);
}

}  // namespace retroui
