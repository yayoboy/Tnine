/* oled.js — emulatore pixel-perfect 128×64 1-bit (stile SSD1306/U8g2)
   Web component: <oled-screen screen="a-home" scale="4" grid="false" paper="false"> */
(function () {
  "use strict";
  var W = 128, H = 64;

  /* ---------- font 5×7 ---------- */
  var FONT = {
    " ": "00000|00000|00000|00000|00000|00000|00000",
    "!": "00100|00100|00100|00100|00100|00000|00100",
    "\"": "01010|01010|01010|00000|00000|00000|00000",
    "#": "01010|01010|11111|01010|11111|01010|01010",
    "%": "11000|11001|00010|00100|01000|10011|00011",
    "&": "01100|10010|10100|01000|10101|10010|01101",
    "'": "00100|00100|00000|00000|00000|00000|00000",
    "(": "00010|00100|01000|01000|01000|00100|00010",
    ")": "01000|00100|00010|00010|00010|00100|01000",
    "+": "00000|00100|00100|11111|00100|00100|00000",
    ",": "00000|00000|00000|00000|01100|00100|01000",
    "-": "00000|00000|00000|11111|00000|00000|00000",
    ".": "00000|00000|00000|00000|00000|01100|01100",
    "/": "00000|00001|00010|00100|01000|10000|00000",
    "0": "01110|10001|10011|10101|11001|10001|01110",
    "1": "00100|01100|00100|00100|00100|00100|01110",
    "2": "01110|10001|00001|00010|00100|01000|11111",
    "3": "11111|00010|00100|00010|00001|10001|01110",
    "4": "00010|00110|01010|10010|11111|00010|00010",
    "5": "11111|10000|11110|00001|00001|10001|01110",
    "6": "00110|01000|10000|11110|10001|10001|01110",
    "7": "11111|00001|00010|00100|01000|01000|01000",
    "8": "01110|10001|10001|01110|10001|10001|01110",
    "9": "01110|10001|10001|01111|00001|00010|01100",
    ":": "00000|01100|01100|00000|01100|01100|00000",
    ";": "00000|01100|01100|00000|01100|00100|01000",
    "<": "00010|00100|01000|10000|01000|00100|00010",
    "=": "00000|00000|11111|00000|11111|00000|00000",
    ">": "01000|00100|00010|00001|00010|00100|01000",
    "?": "01110|10001|00001|00010|00100|00000|00100",
    "@": "01110|10001|10111|10101|10111|10000|01110",
    "A": "01110|10001|10001|11111|10001|10001|10001",
    "B": "11110|10001|10001|11110|10001|10001|11110",
    "C": "01110|10001|10000|10000|10000|10001|01110",
    "D": "11100|10010|10001|10001|10001|10010|11100",
    "E": "11111|10000|10000|11110|10000|10000|11111",
    "F": "11111|10000|10000|11110|10000|10000|10000",
    "G": "01110|10001|10000|10111|10001|10001|01111",
    "H": "10001|10001|10001|11111|10001|10001|10001",
    "I": "01110|00100|00100|00100|00100|00100|01110",
    "J": "00111|00010|00010|00010|00010|10010|01100",
    "K": "10001|10010|10100|11000|10100|10010|10001",
    "L": "10000|10000|10000|10000|10000|10000|11111",
    "M": "10001|11011|10101|10101|10001|10001|10001",
    "N": "10001|10001|11001|10101|10011|10001|10001",
    "O": "01110|10001|10001|10001|10001|10001|01110",
    "P": "11110|10001|10001|11110|10000|10000|10000",
    "Q": "01110|10001|10001|10001|10101|10010|01101",
    "R": "11110|10001|10001|11110|10100|10010|10001",
    "S": "01111|10000|10000|01110|00001|00001|11110",
    "T": "11111|00100|00100|00100|00100|00100|00100",
    "U": "10001|10001|10001|10001|10001|10001|01110",
    "V": "10001|10001|10001|10001|10001|01010|00100",
    "W": "10001|10001|10001|10101|10101|10101|01010",
    "X": "10001|10001|01010|00100|01010|10001|10001",
    "Y": "10001|10001|01010|00100|00100|00100|00100",
    "Z": "11111|00001|00010|00100|01000|10000|11111",
    "[": "01110|01000|01000|01000|01000|01000|01110",
    "]": "01110|00010|00010|00010|00010|00010|01110",
    "_": "00000|00000|00000|00000|00000|00000|11111",
    "a": "00000|00000|01110|00001|01111|10001|01111",
    "b": "10000|10000|11110|10001|10001|10001|11110",
    "c": "00000|00000|01110|10000|10000|10001|01110",
    "d": "00001|00001|01111|10001|10001|10001|01111",
    "e": "00000|00000|01110|10001|11111|10000|01110",
    "f": "00110|01001|01000|11100|01000|01000|01000",
    "g": "00000|01111|10001|10001|01111|00001|01110",
    "h": "10000|10000|11110|10001|10001|10001|10001",
    "i": "00100|00000|01100|00100|00100|00100|01110",
    "j": "00010|00000|00110|00010|00010|10010|01100",
    "k": "10000|10000|10010|10100|11000|10100|10010",
    "l": "01100|00100|00100|00100|00100|00100|01110",
    "m": "00000|00000|11010|10101|10101|10101|10101",
    "n": "00000|00000|11110|10001|10001|10001|10001",
    "o": "00000|00000|01110|10001|10001|10001|01110",
    "p": "00000|00000|11110|10001|11110|10000|10000",
    "q": "00000|00000|01111|10001|01111|00001|00001",
    "r": "00000|00000|10110|11001|10000|10000|10000",
    "s": "00000|00000|01111|10000|01110|00001|11110",
    "t": "01000|01000|11100|01000|01000|01001|00110",
    "u": "00000|00000|10001|10001|10001|10011|01101",
    "v": "00000|00000|10001|10001|10001|01010|00100",
    "w": "00000|00000|10001|10001|10101|10101|01010",
    "x": "00000|00000|10001|01010|00100|01010|10001",
    "y": "00000|00000|10001|10001|01111|00001|01110",
    "z": "00000|00000|11111|00010|00100|01000|11111",
    "°": "01100|10010|10010|01100|00000|00000|00000",
    "·": "00000|00000|01100|01100|00000|00000|00000",
    "±": "00100|00100|11111|00100|00100|00000|11111",
    "▲": "00000|00100|01110|11111|00000|00000|00000",
    "▼": "00000|11111|01110|00100|00000|00000|00000",
    "↑": "00100|01110|10101|00100|00100|00100|00100",
    "↓": "00100|00100|00100|00100|10101|01110|00100",
    "→": "00000|00100|00010|11111|00010|00100|00000"
  };
  var ACC = { "à": "a", "è": "e", "é": "e", "ì": "i", "ò": "o", "ù": "u", "À": "A", "È": "E", "É": "E", "Ì": "I", "Ò": "O", "Ù": "U" };
  var GC = {};
  function glyph(ch) {
    ch = ACC[ch] || ch;
    if (!GC[ch]) GC[ch] = (FONT[ch] || FONT["?"]).split("|");
    return GC[ch];
  }

  /* ---------- icone bitmap ---------- */
  var ENV = ["111111111", "110000011", "101000101", "100101001", "100010001", "100000001", "111111111"];
  var SAT = ["0011100", "0100010", "1000001", "1001001", "1000001", "0100010", "0011100"];
  var ANT = ["1000001", "0100010", "0010100", "0001000", "0001000", "0001000", "0001000"];
  var HOME = ["0001000", "0011100", "0111110", "1111111", "0100010", "0101010", "0111110"];
  var SLD = ["0001100", "1111111", "0001100", "0000000", "0110000", "1111111", "0110000"];

  /* ---------- framebuffer ---------- */
  function FB() { this.b = new Uint8Array(W * H); }
  FB.prototype = {
    px: function (x, y, c) { x |= 0; y |= 0; if (x < 0 || y < 0 || x >= W || y >= H) return; this.b[y * W + x] = c === 0 ? 0 : 1; },
    hline: function (x, y, w, c) { for (var i = 0; i < w; i++) this.px(x + i, y, c); },
    vline: function (x, y, h, c) { for (var i = 0; i < h; i++) this.px(x, y + i, c); },
    dot: function (x, y, w) { for (var i = 0; i < w; i += 3) this.px(x + i, y, 1); },
    rect: function (x, y, w, h, c) { this.hline(x, y, w, c); this.hline(x, y + h - 1, w, c); this.vline(x, y, h, c); this.vline(x + w - 1, y, h, c); },
    fill: function (x, y, w, h, c) { for (var j = 0; j < h; j++) this.hline(x, y + j, w, c); },
    line: function (x0, y0, x1, y1, c) {
      var dx = Math.abs(x1 - x0), dy = -Math.abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy;
      for (; ;) { this.px(x0, y0, c); if (x0 === x1 && y0 === y1) break; var e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
    },
    bmp: function (x, y, art, c, s) {
      s = s || 1;
      for (var j = 0; j < art.length; j++) for (var i = 0; i < art[j].length; i++)
        if (art[j][i] === "1") this.fill(x + i * s, y + j * s, s, s, c);
    },
    text: function (x, y, str, s, c) {
      s = s || 1;
      for (var k = 0; k < str.length; k++) {
        var g = glyph(str[k]);
        for (var j = 0; j < 7; j++) for (var i = 0; i < 5; i++)
          if (g[j][i] === "1") this.fill(x + k * 6 * s + i * s, y + j * s, s, s, c);
      }
      return str.length * 6 * s - s;
    },
    textW: function (str, s) { return str.length * 6 * (s || 1) - (s || 1); },
    textC: function (cx, y, str, s, c) { this.text(cx - Math.round(this.textW(str, s) / 2), y, str, s, c); },
    textR: function (xr, y, str, s, c) { this.text(xr - this.textW(str, s), y, str, s, c); },
    batt: function (x, y, pct, c) {
      this.rect(x, y, 11, 6, c); this.vline(x + 11, y + 2, 2, c);
      var w = Math.round(9 * pct / 100); if (w > 0) this.fill(x + 1, y + 1, w, 4, c);
    },
    bars: function (x, y, n, step, c) {
      step = step || 1;
      for (var i = 0; i < 4; i++) {
        var h = 2 + i * step, bx = x + i * 3;
        if (i < n) this.fill(bx, y - h + 1, 2, h, c); else { this.px(bx, y, c); this.px(bx + 1, y, c); }
      }
    },
    scroll: function (x, y, h, pos, total, vis) {
      for (var j = 0; j < h; j += 3) this.px(x, y + j, 1);
      var th = Math.max(4, Math.round(h * vis / total));
      var ty = y + Math.round((h - th) * pos / Math.max(1, total - vis));
      this.fill(x - 1, ty, 3, th, 1);
    }
  };

  /* ---------- helper condivisi ---------- */
  function statusbarA(fb) {
    fb.text(0, 0, "12:47");
    fb.bmp(63, 0, ENV);
    fb.bmp(76, 0, SAT);
    fb.bars(88, 6, 3);
    fb.textR(113, 0, "87");
    fb.batt(116, 0, 87);
    fb.hline(0, 8, 128);
  }
  function headerA(fb, sx, dx) {
    fb.fill(0, 0, 128, 9);
    fb.text(2, 1, sx, 1, 0);
    if (dx) fb.textR(126, 1, dx, 1, 0);
  }
  function hintA(fb, txt) {
    fb.fill(0, 55, 128, 9);
    fb.textC(64, 56, txt, 1, 0);
  }
  function dotsB(fb, sel, n) {
    n = n || 6;
    var w = n * 6 - 3, x0 = 64 - Math.round(w / 2);
    for (var i = 0; i < n; i++) {
      if (i === sel) fb.fill(x0 + i * 6, 58, 3, 3);
      else fb.px(x0 + i * 6 + 1, 59);
    }
  }
  function tabsC(fb, sel) {
    fb.hline(0, 52, 128);
    var icons = [HOME, ANT, ENV, SAT, SLD], xs = [12, 38, 64, 90, 116];
    for (var i = 0; i < 5; i++) {
      var iw = icons[i][0].length, x = xs[i] - Math.floor(iw / 2);
      if (i === sel) { fb.fill(xs[i] - 10, 54, 21, 10); fb.bmp(x, 55, icons[i], 0); }
      else fb.bmp(x, 55, icons[i]);
    }
  }

  /* ---------- schermate ---------- */
  var S = {};

  /* — variante A: densa — */
  S["a-boot"] = function (fb) {
    fb.rect(0, 0, 128, 64);
    fb.textC(64, 10, "RADURA", 2);
    fb.textC(64, 29, "MESH NODE · v1.0.2");
    fb.rect(24, 41, 80, 7);
    fb.fill(26, 43, 50, 3);
    fb.textC(64, 53, "AVVIO RADIO...");
  };
  S["a-home"] = function (fb) {
    statusbarA(fb);
    fb.text(0, 12, "NODI 14"); fb.text(66, 12, "ONLINE 6");
    fb.text(0, 21, "UTIL 31%"); fb.text(66, 21, "AIR 2.4%");
    fb.text(0, 30, "TX 142"); fb.text(66, 30, "RX 1.2K");
    fb.text(0, 39, "CH LONGFAST"); fb.text(94, 39, "SF11");
    fb.dot(0, 50, 128);
    hintA(fb, "↑↓ PAGINA · OK MENU");
  };
  S["a-nodes"] = function (fb) {
    headerA(fb, "NODI", "4/14");
    var rows = [["MARCO-1", 4, "2M"], ["BASE-SUD", 3, "5M"], ["!A4F2C", 2, "1H"], ["RIFUGIO", 2, "3H"], ["GW-NORD", 1, "1G"]];
    for (var i = 0; i < 5; i++) {
      var y = 12 + i * 10, sel = i === 1, c = sel ? 0 : 1;
      if (sel) fb.fill(0, y - 1, 124, 9);
      fb.text(2, y, rows[i][0], 1, c);
      fb.bars(76, y + 6, rows[i][1], 1, c);
      fb.textR(122, y, rows[i][2], 1, c);
    }
    fb.scroll(126, 11, 52, 1, 14, 5);
  };
  S["a-msgs"] = function (fb) {
    headerA(fb, "MESSAGGI", "1 NUOVO");
    fb.fill(1, 14, 3, 3);
    fb.text(7, 12, "MARCO-1"); fb.textR(126, 12, "12:41");
    fb.text(7, 21, "Ci vediamo alle 18");
    fb.dot(0, 32, 128);
    fb.text(7, 36, "!A4F2C"); fb.textR(126, 36, "11:07");
    fb.text(7, 45, "Ok ricevuto, grazie");
    hintA(fb, "↑↓ SCORRI · OK APRI");
  };
  S["a-comp"] = function (fb) {
    headerA(fb, "COMPONI", "→ MARCO-1");
    var items = ["Ok", "Arrivo tra poco", "Posizione?", "Si", "No", "SOS"];
    for (var i = 0; i < 5; i++) {
      var y = 12 + i * 9, sel = i === 2;
      if (sel) fb.fill(0, y - 1, 124, 9);
      fb.text(4, y, items[i], 1, sel ? 0 : 1);
    }
    fb.scroll(126, 11, 42, 2, 6, 5);
    hintA(fb, "OK INVIA · < ESCI");
  };
  S["a-gps"] = function (fb) {
    headerA(fb, "GPS", "FIX 3D · 8 SAT");
    fb.text(2, 13, "LAT 45.46421°");
    fb.text(2, 22, "LON 9.19003°");
    fb.text(2, 31, "ALT 224M"); fb.text(72, 31, "±4M");
    fb.text(2, 40, "VEL 4.2KM/H"); fb.text(96, 40, "NE");
    fb.dot(0, 50, 128);
    hintA(fb, "OK CONDIVIDI POS");
  };
  S["a-cfg"] = function (fb) {
    headerA(fb, "CONFIG");
    var rows = [["Canale", "LongFast"], ["Regione", "EU868"], ["Potenza", "20dBm"], ["Schermo", "30s"], ["Ruolo", "CLIENT"]];
    for (var i = 0; i < 5; i++) {
      var y = 12 + i * 9, sel = i === 2, c = sel ? 0 : 1;
      if (sel) fb.fill(0, y - 1, 124, 9);
      fb.text(2, y, rows[i][0], 1, c);
      fb.textR(114, y, rows[i][1], 1, c);
      fb.text(118, y, ">", 1, c);
    }
    fb.scroll(126, 11, 42, 2, 8, 5);
    hintA(fb, "OK MODIFICA");
  };

  /* — variante B: minimale — */
  S["b-boot"] = function (fb) {
    fb.textC(64, 20, "RADURA", 2);
    fb.textC(64, 42, "v1.0.2");
  };
  S["b-home"] = function (fb) {
    fb.textC(64, 3, "RADURA-1");
    fb.hline(48, 12, 32);
    fb.textC(64, 19, "14 NODI", 2);
    fb.textC(64, 40, "BATT 87% · GPS OK");
    dotsB(fb, 0);
  };
  S["b-nodes"] = function (fb) {
    fb.textC(64, 3, "NODI · 3/14");
    fb.hline(48, 12, 32);
    fb.textC(64, 19, "MARCO-1", 2);
    fb.textC(64, 40, "RSSI -92 · 2M FA");
    dotsB(fb, 1);
  };
  S["b-msgs"] = function (fb) {
    fb.textC(64, 3, "MARCO-1 · 12:41");
    fb.textC(64, 15, "CI VEDIAMO", 2);
    fb.textC(64, 31, "ALLE 18", 2);
    fb.textC(64, 49, "1/3 · OK RISPONDI");
    dotsB(fb, 2);
  };
  S["b-comp"] = function (fb) {
    fb.textC(64, 3, "A MARCO-1 · 3/6");
    fb.hline(48, 12, 32);
    fb.textC(64, 19, "POSIZIONE?", 2);
    fb.textC(64, 40, "↑↓ ALTRI · OK INVIA");
    dotsB(fb, 3);
  };
  S["b-gps"] = function (fb) {
    fb.textC(64, 8, "45.4642 N", 2);
    fb.textC(64, 26, "9.1900 E", 2);
    fb.textC(64, 46, "8 SAT · ±4M · 224M");
    dotsB(fb, 4);
  };
  S["b-cfg"] = function (fb) {
    fb.textC(64, 3, "CONFIG · CANALE");
    fb.hline(48, 12, 32);
    fb.textC(64, 19, "LONGFAST", 2);
    fb.textC(64, 40, "OK MODIFICA");
    dotsB(fb, 5);
  };

  /* — variante C: icone — */
  S["c-boot"] = function (fb) {
    var p = [[34, 20], [64, 32], [94, 14]];
    fb.line(p[0][0], p[0][1], p[1][0], p[1][1]);
    fb.line(p[1][0], p[1][1], p[2][0], p[2][1]);
    fb.line(p[0][0], p[0][1], p[2][0], p[2][1]);
    for (var i = 0; i < 3; i++) {
      var x = p[i][0], y = p[i][1];
      fb.fill(x - 3, y - 3, 7, 7, 0);
      fb.fill(x - 1, y - 1, 3, 3);
      fb.rect(x - 3, y - 3, 7, 7);
    }
    fb.textC(64, 44, "RADURA", 2);
  };
  S["c-home"] = function (fb) {
    fb.vline(64, 0, 52); fb.hline(0, 26, 128);
    fb.batt(6, 10, 87); fb.text(24, 10, "87%", 2);
    fb.bmp(72, 9, ANT); fb.text(86, 6, "14", 2);
    fb.bmp(4, 36, ENV); fb.text(24, 33, "3", 2);
    fb.bmp(72, 36, SAT); fb.text(86, 33, "8", 2);
    tabsC(fb, 0);
  };
  S["c-nodes"] = function (fb) {
    fb.text(2, 1, "NODI"); fb.textR(126, 1, "14");
    var rows = [["MARCO-1", 4, "2M"], ["BASE-SUD", 3, "5M"], ["RIFUGIO", 2, "3H"]];
    for (var i = 0; i < 3; i++) {
      var y = 11 + i * 13;
      if (i === 1) fb.rect(0, y, 128, 12);
      fb.bars(5, y + 8, rows[i][1]);
      fb.text(20, y + 3, rows[i][0]);
      fb.textR(122, y + 3, rows[i][2]);
    }
    tabsC(fb, 1);
  };
  S["c-msgs"] = function (fb) {
    fb.text(2, 1, "MESSAGGI"); fb.textR(126, 1, "3");
    fb.rect(2, 11, 116, 27);
    fb.fill(6, 14, 45, 9);
    fb.text(8, 15, "MARCO-1", 1, 0);
    fb.text(6, 27, "Ci vediamo alle 18");
    fb.line(14, 38, 14, 43); fb.line(14, 43, 19, 38);
    fb.textR(118, 42, "12:41");
    tabsC(fb, 2);
  };
  S["c-comp"] = function (fb) {
    fb.text(2, 1, "INVIA → MARCO-1");
    var items = ["Ok", "Arrivo", "Posiz?", "Si", "No", "SOS"];
    for (var i = 0; i < 6; i++) {
      var x = i % 2 ? 66 : 1, y = 11 + Math.floor(i / 2) * 14, sel = i === 2;
      if (sel) { fb.fill(x, y, 61, 12); fb.textC(x + 30, y + 3, items[i], 1, 0); }
      else { fb.rect(x, y, 61, 12); fb.textC(x + 30, y + 3, items[i]); }
    }
    tabsC(fb, 2);
  };
  S["c-gps"] = function (fb) {
    fb.bmp(8, 14, SAT, 1, 3);
    fb.text(38, 12, "45.4642 N");
    fb.text(38, 21, "9.1900 E");
    fb.text(38, 30, "224M · ±4M");
    var hs = [3, 7, 4, 6, 2, 5, 7, 3];
    for (var i = 0; i < 8; i++) fb.fill(38 + i * 6, 47 - hs[i], 2, hs[i]);
    fb.text(92, 40, "8 SAT");
    tabsC(fb, 3);
  };
  S["c-cfg"] = function (fb) {
    fb.text(2, 1, "CONFIG");
    var rows = [["Canale", "LongFast"], ["Potenza", "20dBm"], ["Schermo", "30s"]];
    for (var i = 0; i < 3; i++) {
      var y = 11 + i * 13;
      if (i === 1) fb.rect(0, y, 128, 12);
      fb.text(5, y + 3, rows[i][0]);
      fb.textR(122, y + 3, rows[i][1]);
    }
    tabsC(fb, 4);
  };

  /* — variante D: retro '84 (kit bitmap) — */
  var CIRC = ["0011100", "0100010", "1000001", "1000001", "1000001", "0100010", "0011100"];
  var LOUPE = ["01100", "10010", "10010", "01101", "00011"];
  function hatch(fb, x, y, w, h) { for (var j = 0; j < h; j++) for (var i = 0; i < w; i++) if ((i + j) % 2 === 0) fb.px(x + i, y + j, 1); }
  function rr(fb, x, y, w, h) { fb.hline(x + 1, y, w - 2, 1); fb.hline(x + 1, y + h - 1, w - 2, 1); fb.vline(x, y + 1, h - 2, 1); fb.vline(x + w - 1, y + 1, h - 2, 1); }
  function win(fb, x, y, w, h, title, rtxt) {
    fb.rect(x, y, w, h); fb.hline(x, y + 10, w); fb.text(x + 3, y + 2, title);
    var sx = x + 3 + fb.textW(title, 1) + 6, ex = x + w - 4;
    if (rtxt) { fb.textR(x + w - 4, y + 2, rtxt); ex = x + w - 4 - fb.textW(rtxt, 1) - 6; }
    if (ex > sx) for (var j = 0; j < 4; j++) fb.hline(sx, y + 2 + j * 2, ex - sx + 1);
  }
  function vscrollR(fb, x, y, h, pos, total, vis) {
    fb.rect(x, y, 7, h);
    var th = Math.max(6, Math.round((h - 2) * vis / total));
    var ty = y + 1 + Math.round((h - 2 - th) * pos / Math.max(1, total - vis));
    hatch(fb, x + 1, ty, 5, th);
  }
  function gauge(fb, x, y, w, pct) {
    fb.rect(x, y, w, 5);
    var fw = Math.round((w - 2) * pct / 100);
    hatch(fb, x + 1, y + 1, fw, 3);
    var cx = x + 1 + fw;
    fb.fill(cx - 2, y, 5, 5, 0);
    fb.px(cx, y); fb.hline(cx - 1, y + 1, 3); fb.hline(cx - 2, y + 2, 5); fb.hline(cx - 1, y + 3, 3); fb.px(cx, y + 4);
  }
  function bubble(fb, x, y, txt) {
    var w = fb.textW(txt, 1) + 8;
    rr(fb, x, y, w, 11); fb.text(x + 4, y + 2, txt);
    fb.px(x - 1, y + 5); fb.px(x - 2, y + 6);
  }
  function stepper(fb, x, y, val) {
    rr(fb, x, y, 36, 11);
    fb.vline(x + 11, y + 1, 9); fb.vline(x + 24, y + 1, 9);
    fb.hline(x + 3, y + 5, 5);
    fb.textC(x + 18, y + 2, val);
    fb.hline(x + 27, y + 5, 5); fb.vline(x + 29, y + 3, 5);
  }
  function toggle(fb, x, y, on) {
    rr(fb, x, y, 16, 9);
    if (on) fb.fill(x + 9, y + 2, 5, 5); else fb.rect(x + 2, y + 2, 5, 5);
  }
  function checkbox(fb, x, y, ck) {
    fb.rect(x, y, 7, 7);
    if (ck) { fb.line(x + 1, y + 3, x + 3, y + 5); fb.line(x + 3, y + 5, x + 5, y + 1); }
  }
  function radio(fb, x, y, on) { fb.bmp(x, y, CIRC); if (on) fb.fill(x + 2, y + 2, 3, 3); }
  function triU(fb, x, y) { fb.px(x + 2, y); fb.hline(x + 1, y + 1, 3); fb.hline(x, y + 2, 5); }
  function triD(fb, x, y) { fb.hline(x, y, 5); fb.hline(x + 1, y + 1, 3); fb.px(x + 2, y + 2); }
  function rrbtn(fb, x, y, w, h, txt, sel) {
    rr(fb, x, y, w, h);
    if (sel) { fb.fill(x + 1, y + 1, w - 2, h - 2); fb.textC(x + Math.round(w / 2), y + 3, txt, 1, 0); }
    else fb.textC(x + Math.round(w / 2), y + 3, txt);
  }
  function steps(fb, x, y, w, a, b) {
    fb.rect(x, y, w, 13);
    var mid = x + Math.round(w / 2);
    for (var j = 0; j < 11; j++) { var ext = j < 6 ? j : 10 - j; fb.hline(x + 1, y + 1 + j, (mid - x - 6) + ext); }
    fb.textC(x + Math.round((mid - x) / 2) - 2, y + 3, a, 1, 0);
    fb.textC(mid + Math.round((x + w - mid) / 2) + 2, y + 3, b);
  }
  S["d-boot"] = function (fb) {
    win(fb, 0, 0, 128, 64, "RADURA OS", "v1.0");
    fb.textC(64, 17, "RADURA", 2);
    gauge(fb, 8, 42, 84, 62);
    bubble(fb, 100, 39, "62%");
    fb.text(8, 53, "AVVIO RADIO...");
  };
  S["d-home"] = function (fb) {
    win(fb, 0, 0, 128, 64, "RADURA-1", "12:47");
    fb.text(4, 14, "BATT"); gauge(fb, 34, 15, 60, 87); fb.textR(124, 14, "87%");
    fb.text(4, 25, "UTIL"); gauge(fb, 34, 26, 60, 31); fb.textR(124, 25, "31%");
    fb.text(4, 37, "NODI 14"); fb.textR(124, 37, "GPS OK");
    fb.dot(4, 47, 120);
    fb.textC(64, 51, "CH LONGFAST · SF11");
  };
  S["d-nodes"] = function (fb) {
    win(fb, 0, 0, 128, 64, "NODI", "14");
    var rows = [["MARCO-1", 4, "2M"], ["BASE-SUD", 3, "5M"], ["RIFUGIO", 2, "3H"]];
    for (var i = 0; i < 3; i++) {
      var y = 13 + i * 12, sel = i === 1, c = sel ? 0 : 1;
      if (sel) fb.fill(2, y - 1, 114, 11);
      fb.text(4, y, rows[i][0], 1, c);
      fb.bars(76, y + 6, rows[i][1], 1, c);
      fb.textR(112, y, rows[i][2], 1, c);
    }
    vscrollR(fb, 118, 13, 36, 0, 14, 3);
    rr(fb, 3, 50, 100, 12); fb.text(7, 52, "cerca_");
    rr(fb, 106, 50, 12, 12); fb.bmp(109, 53, LOUPE);
  };
  S["d-msgs"] = function (fb) {
    win(fb, 0, 0, 128, 64, "MARCO-1", "12:41");
    fb.text(4, 13, "Ci vediamo alle");
    fb.text(4, 22, "18 davanti al");
    fb.text(4, 31, "rifugio, porta");
    fb.text(4, 40, "la corda!");
    vscrollR(fb, 118, 13, 32, 0, 6, 4);
    rrbtn(fb, 3, 48, 58, 13, "RISPONDI", 1);
    rrbtn(fb, 66, 48, 46, 13, "CHIUDI", 0);
  };
  S["d-comp"] = function (fb) {
    win(fb, 0, 0, 128, 64, "COMPONI", "MARCO-1");
    fb.text(4, 14, "MSG RAPIDO 3/6");
    rr(fb, 3, 24, 110, 14); fb.text(8, 28, "Posizione?");
    fb.vline(98, 25, 12); triU(fb, 102, 28); triD(fb, 102, 34);
    steps(fb, 3, 44, 110, "TESTO", "INVIA");
  };
  S["d-gps"] = function (fb) {
    win(fb, 0, 0, 128, 64, "GPS", "FIX 3D");
    fb.text(4, 14, "45.4642N");
    fb.text(4, 23, "9.1900E");
    fb.text(4, 32, "ALT 224M");
    fb.text(4, 41, "±4M");
    fb.rect(62, 13, 62, 38);
    for (var gx = 1; gx < 5; gx++) for (var gy = 15; gy < 49; gy += 3) fb.px(62 + gx * 12, gy);
    for (var gj = 1; gj < 4; gj++) for (var gi = 64; gi < 122; gi += 3) fb.px(gi, 13 + gj * 9);
    var hs = [12, 30, 18, 34, 10, 24];
    for (var i = 0; i < 6; i++) {
      var bx = 66 + i * 9, bh = hs[i];
      if (i % 2) hatch(fb, bx, 49 - bh, 4, bh); else fb.fill(bx, 49 - bh, 4, bh);
      fb.rect(bx, 49 - bh, 4, bh);
    }
    fb.text(4, 53, "8 SAT · HDOP 1.2");
  };
  S["d-cfg"] = function (fb) {
    win(fb, 0, 0, 128, 64, "CONFIG");
    fb.text(4, 14, "GPS"); fb.textR(102, 14, "ON"); toggle(fb, 108, 12, true);
    fb.text(4, 25, "BEEP TX"); checkbox(fb, 112, 24, true);
    fb.text(4, 38, "POTENZA"); stepper(fb, 86, 34, "20");
    fb.text(4, 50, "RUOLO"); radio(fb, 46, 49, true); fb.text(57, 50, "CLI"); radio(fb, 84, 49, false); fb.text(95, 50, "RTR");
  };

  /* ---------- web component ---------- */
  var OledScreen = function () { return Reflect.construct(HTMLElement, [], OledScreen); };
  OledScreen.prototype = Object.create(HTMLElement.prototype);
  Object.defineProperty(OledScreen, "observedAttributes", { get: function () { return ["screen", "scale", "grid", "paper"]; } });

  function toBool(v) { return v === true || v === "true" || v === ""; }
  function toScale(v) { var n = parseInt(v, 10); return isNaN(n) ? 4 : Math.min(12, Math.max(1, n)); }

  OledScreen.prototype._st = null;
  OledScreen.prototype._state = function () {
    if (!this._st) this._st = { screen: "", scale: 4, grid: false, paper: false };
    return this._st;
  };
  OledScreen.prototype._set = function (n, v) {
    var st = this._state();
    if (n === "scale") v = toScale(v);
    else if (n === "grid" || n === "paper") v = toBool(v);
    st[n] = v;
    this._draw();
  };
  OledScreen.prototype.attributeChangedCallback = function (n, o, v) { this._set(n, v); };
  ["screen", "scale", "grid", "paper"].forEach(function (p) {
    Object.defineProperty(OledScreen.prototype, p, {
      get: function () { return this._state()[p]; },
      set: function (v) { this._set(p, v); }
    });
  });
  OledScreen.prototype.connectedCallback = function () {
    if (!this._c) {
      this._c = document.createElement("canvas");
      this._c.style.display = "block";
      this._c.style.imageRendering = "pixelated";
      this.appendChild(this._c);
    }
    this._draw();
  };
  OledScreen.prototype._draw = function () {
    if (!this._c) return;
    var st = this._state(), s = st.scale, c = this._c;
    c.width = W * s; c.height = H * s;
    var g = c.getContext("2d");
    var bg = st.paper ? "#ffffff" : "#0c1015";
    var lit = st.paper ? "#17181c" : "#e4f2ff";
    g.fillStyle = bg; g.fillRect(0, 0, c.width, c.height);
    if (st.grid && s >= 3) {
      g.fillStyle = st.paper ? "rgba(0,0,0,0.10)" : "rgba(160,200,255,0.10)";
      for (var gx = 0; gx <= W; gx++) g.fillRect(gx * s, 0, 1, H * s);
      for (var gy = 0; gy <= H; gy++) g.fillRect(0, gy * s, c.width, 1);
    }
    var fb = new FB(), fn = S[st.screen];
    if (fn) fn(fb);
    g.fillStyle = lit;
    var inset = st.grid && s >= 3 ? 1 : 0;
    for (var y = 0; y < H; y++) for (var x = 0; x < W; x++)
      if (fb.b[y * W + x]) g.fillRect(x * s + inset, y * s + inset, s - inset, s - inset);
  };

  if (!customElements.get("oled-screen")) customElements.define("oled-screen", OledScreen);
})();
