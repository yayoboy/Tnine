# Tnine – Tastiera T9 per Meshtastic (RP2040)

Firmware per RP2040 che trasforma un tastierino telefonico 4×3 in una
tastiera T9 (multi-tap) per inviare messaggi di testo attraverso una board
Meshtastic, collegata via UART tramite il **serial module** in modalità
`TEXTMSG`.

- **OLED 0.42"** (SSD1306 72×40, I2C0): anteprima in grande del carattere
  che si sta componendo con il multi-tap.
- **OLED 128×64** (SSD1306, I2C1): interfaccia utente con modalità corrente,
  contatore caratteri, messaggio in composizione e ultimo messaggio ricevuto
  dalla rete mesh.

## Hardware

- Raspberry Pi Pico o altra board RP2040
- Tastierino a matrice 4 righe × 3 colonne (layout telefonico `1-9, *, 0, #`)
- OLED 0.42" SSD1306 72×40 I2C (indirizzo 0x3C)
- OLED 0.96"/1.3" SSD1306 128×64 I2C (indirizzo 0x3C)
- Nodo Meshtastic con serial module disponibile (es. T-Beam, Heltec, RAK)

### Collegamenti (Raspberry Pi Pico)

| Funzione                  | Pin RP2040 |
|---------------------------|------------|
| OLED 0.42" SDA (I2C0)     | GP4        |
| OLED 0.42" SCL (I2C0)     | GP5        |
| OLED 128×64 SDA (I2C1)    | GP6        |
| OLED 128×64 SCL (I2C1)    | GP7        |
| UART TX → RXD Meshtastic  | GP8        |
| UART RX ← TXD Meshtastic  | GP9        |
| Righe tastierino R1–R4    | GP10–GP13  |
| Colonne tastierino C1–C3  | GP14–GP16  |

> Nota: collegare anche GND in comune tra RP2040 e nodo Meshtastic.
> I pin sono configurabili in `src/config.h`.

## Configurazione del nodo Meshtastic

Abilitare il serial module in modalità `TEXTMSG` (esempio con la CLI,
adattare i pin RXD/TXD alla propria board):

```bash
meshtastic --set serial.enabled true \
           --set serial.mode TEXTMSG \
           --set serial.baud BAUD_38400 \
           --set serial.rxd <GPIO collegato a GP8> \
           --set serial.txd <GPIO collegato a GP9>
```

In questa modalità ogni riga di testo ricevuta sulla UART viene trasmessa
come messaggio sul canale primario, e i messaggi ricevuti vengono emessi
come righe di testo.

## Uso della tastiera

| Tasto      | Azione                                            |
|------------|---------------------------------------------------|
| `0`–`9`    | Input T9 multi-tap (`2` = a→b→c→2, `0` = spazio…) |
| `1`        | Punteggiatura: `. , ? ! ' " - @ / : 1`            |
| `*` breve  | Backspace (o annulla il carattere candidato)      |
| `*` lungo  | Cancella tutto il messaggio                       |
| `#` breve  | Invia il messaggio                                |
| `#` lungo  | Cambia modalità: `abc` → `ABC` → `123`            |

Il carattere candidato viene confermato automaticamente dopo 800 ms o
premendo un tasto diverso.

## Compilazione

Il progetto usa [PlatformIO](https://platformio.org/) con il core
[arduino-pico](https://github.com/earlephilhower/arduino-pico):

```bash
pio run                 # compila
pio run -t upload       # carica sul Pico (BOOTSEL o picotool)
pio device monitor      # log di debug via USB a 115200 baud
```

## Struttura del codice

```
src/
├── main.cpp            # ciclo principale e gestione dei tasti
├── config.h            # pin, baud rate, timing
├── MatrixKeypad.*      # scansione matrice 4×3, debounce, pressione lunga
├── T9Engine.*          # logica multi-tap T9 e buffer del messaggio
├── MeshtasticLink.*    # UART verso il serial module (TEXTMSG)
├── PreviewDisplay.*    # OLED 0.42": anteprima carattere
└── UIDisplay.*         # OLED 128×64: interfaccia utente
```
