# Tnine – Tastiera T9 per Meshtastic (RP2040)

Firmware per RP2040 che trasforma un tastierino telefonico 4×3 in una
tastiera T9 (multi-tap) per inviare e ricevere messaggi di testo su una rete
[Meshtastic](https://meshtastic.org), collegandosi a un nodo via UART tramite
il **serial module in modalità `PROTO`** (client API protobuf).

```
┌─────────────┐   I2C0   ┌──────────────────┐
│ OLED 0.42"  │◄─────────┤                  │  UART1   ┌──────────────────┐
│ (anteprima) │          │      RP2040      ├─────────►│  Nodo Meshtastic │
└─────────────┘   I2C1   │                  │◄─────────┤  (serial PROTO)  │
┌─────────────┐◄─────────┤                  │          └──────────────────┘
│ OLED 128×64 │          └───────┬──────────┘
│    (UI)     │                  │ matrice 4×3
└─────────────┘          ┌───────┴──────────┐
                         │  1 2 3 / 4 5 6   │
                         │  7 8 9 / * 0 #   │
                         └──────────────────┘
```

## Caratteristiche

- **UI "Retro '84"**: interfaccia a finestre bitmap (barra titolo a righe,
  scrollbar tratteggiate, gauge con pomello, pulsanti arrotondati) fedele al
  prototipo pixel-perfect in `docs/ui-prototype/`.
- **Input T9 multi-tap** in stile telefonico, con lettere accentate italiane
  (`à è é ì ò ù`) e tre modalità: minuscole, maiuscole, numeri.
- **Anteprima del carattere** in grande sull'OLED 0.42" mentre si compone.
- **Messaggi rapidi** predefiniti ("Ok", "Arrivo tra poco", "Posizione?",
  "Aiuto! SOS"…) selezionabili dal menu e modificabili prima dell'invio.
- **Messaggi diretti**: destinatario selezionabile dalla lista dei nodi
  della mesh (broadcast di default), con nomi presi dal database nodi.
- **Selezione canale**: lista dei canali configurati sul nodo, con i loro
  nomi.
- **Conferme di consegna**: ACK/NAK end-to-end con motivo dell'errore
  (`consegnato`, `timeout`, `no route`…).
- **Storico messaggi**: gli ultimi 8 messaggi ricevuti con contatore dei non
  letti, vista dettaglio scorrevole e pulsante **RISPONDI** che imposta il
  mittente come destinatario; le **posizioni** ricevute vengono decodificate
  in coordinate.
- **Stato del nodo**: batteria (o alimentazione USB) nella barra di stato,
  schermata Info con id nodo, collegamento, nodi noti.
- **Sessione robusta**: handshake `want_config_id`, heartbeat periodico e
  rinegoziazione automatica quando il nodo Meshtastic si riavvia.
- **Zero dipendenze protobuf**: encoder/decoder minimale scritto a mano,
  interamente coperto da test nativi eseguibili su PC.

## Hardware

| Componente | Note |
|---|---|
| Board RP2040 | Raspberry Pi Pico o equivalente |
| Tastierino a matrice 4×3 | layout telefonico `1-9, *, 0, #` |
| OLED 0.42" SSD1306 72×40 I2C | indirizzo 0x3C |
| OLED 0.96"/1.3" SSD1306 128×64 I2C | indirizzo 0x3C |
| Nodo Meshtastic | qualsiasi board con serial module (T-Beam, Heltec, RAK…) |

### Collegamenti (Raspberry Pi Pico)

| Funzione | Pin RP2040 |
|---|---|
| OLED 0.42" SDA (I2C0) | GP4 |
| OLED 0.42" SCL (I2C0) | GP5 |
| OLED 128×64 SDA (I2C1) | GP6 |
| OLED 128×64 SCL (I2C1) | GP7 |
| UART TX → RXD Meshtastic | GP8 |
| UART RX ← TXD Meshtastic | GP9 |
| Righe tastierino R1–R4 | GP10–GP13 |
| Colonne tastierino C1–C3 | GP14–GP16 |

> **Importante**: collegare GND in comune tra RP2040 e nodo Meshtastic.
> Entrambi i dispositivi lavorano a 3.3 V, nessun level shifter necessario.
> Tutti i pin sono configurabili in [`src/config.h`](src/config.h).

## Configurazione del nodo Meshtastic

Abilitare il serial module in modalità **PROTO** (esempio con la
[CLI Python](https://meshtastic.org/docs/software/python/cli/); i pin RXD/TXD
sono i GPIO della board Meshtastic collegati rispettivamente a GP8 e GP9 del
Pico):

```bash
meshtastic --set serial.enabled true \
           --set serial.mode PROTO \
           --set serial.baud BAUD_38400 \
           --set serial.rxd <GPIO collegato a GP8> \
           --set serial.txd <GPIO collegato a GP9>
```

In modalità `PROTO` i pin del serial module parlano lo stesso protocollo
protobuf del client API usato da app e CLI: la tastiera riceve quindi
configurazione, canali, nodi noti, telemetria, ACK di consegna e messaggi.

## Uso della tastiera

### Composizione

| Tasto | Azione |
|---|---|
| `2`–`9` | Lettere multi-tap (`2` = a→b→c→2→à) |
| `1` | Punteggiatura: `. , ? ! ' " - @ / : 1` |
| `0` | Spazio, poi `0` |
| `0` lungo | Apre il **menu** |
| `*` breve | Backspace (o annulla il carattere candidato) |
| `*` lungo | Cancella tutto il messaggio |
| `#` breve | Invia al destinatario/canale selezionati |
| `#` lungo | Cambia modalità: `abc` → `ABC` → `123` |

### Menu e liste

| Tasto | Azione |
|---|---|
| `2` / `8` | Su / giù (nel dettaglio: scorri il testo) |
| `4` / `6` | Nel dettaglio messaggio: cambia pulsante (RISPONDI/CHIUDI) |
| `5` o `#` | Seleziona / attiva il pulsante |
| `*` | Indietro |

Voci del menu: **Destinatario** (broadcast o un nodo della mesh, con barre
di segnale da SNR), **Canale** (tra quelli configurati sul nodo),
**Messaggi** (storico con dettaglio e risposta rapida), **Msg rapidi**
(frasi predefinite caricate nell'editor), **Info** (id nodo, gauge
batteria, stato collegamento).

All'accensione una schermata di avvio mostra l'avanzamento della
connessione al nodo; qualsiasi tasto la salta.

Sequenze complete dei tasti:

| Tasto | Sequenza |
|---|---|
| `2` | a b c 2 à |
| `3` | d e f 3 è é |
| `4` | g h i 4 ì |
| `5` | j k l 5 |
| `6` | m n o 6 ò |
| `7` | p q r s 7 |
| `8` | t u v 8 ù |
| `9` | w x y z 9 |

Il carattere candidato è confermato automaticamente dopo 800 ms o premendo
un tasto diverso. Dopo l'invio la barra di stato mostra `invio...` e poi
l'esito dell'ACK: `consegnato`, oppure il motivo dell'errore (`timeout`,
`no route`, `max ritx`…).

## Compilazione

Il progetto usa [PlatformIO](https://platformio.org/) con il core
[arduino-pico](https://github.com/earlephilhower/arduino-pico) e la libreria
[U8g2](https://github.com/olikraus/u8g2):

```bash
pio run                 # compila
pio run -t upload       # carica sul Pico (BOOTSEL o picotool)
pio device monitor      # log di debug via USB a 115200 baud
```

In alternativa, la GitHub Action **Build** compila a ogni push e pubblica il
`firmware.uf2` pronto da copiare sul Pico (tenere premuto BOOTSEL,
collegare via USB, trascinare il file sul disco `RPI-RP2`).

## Test

La logica pura (T9, protobuf, framing) non dipende da Arduino e si testa
direttamente su PC:

```bash
tests/run_tests.sh
```

I test coprono: multi-tap con accentate UTF-8, cambio modalità, backspace su
caratteri multi-byte, limiti di lunghezza in byte, roundtrip
encoder/decoder protobuf (varint, bytes, fixed32), struttura dei frame
`ToRadio` (destinatario, canale, want_ack), risincronizzazione del deframer
su dati corrotti e parsing degli eventi `FromRadio` (MyNodeInfo, NodeInfo
con SNR/batteria/last_heard, canali, messaggi di testo, posizioni,
telemetria, ACK/NAK, config complete, rebooted).

## Architettura

```
src/
├── main.cpp              # ciclo principale, schermate, menu, storico
├── config.h              # pin, baud rate, timing, limiti
├── MatrixKeypad.*        # scansione matrice 4×3, debounce, pressione lunga
├── T9Engine.*            # multi-tap T9 UTF-8 e buffer del messaggio
├── MeshtasticClient.*    # client API: sessione, nodi, canali, ACK (Arduino)
├── mesh/                 # logica pura, testabile in nativo
│   ├── ProtoWriter.*     # encoder protobuf minimale (varint + bytes)
│   ├── ProtoReader.*     # decoder protobuf minimale (varint, bytes, fixed32)
│   └── MeshtasticCodec.* # frame 0x94C3, ToRadio/FromRadio, deframer
├── RetroUI.*             # kit widget "Retro '84" su U8g2 (dal prototipo)
├── PreviewDisplay.*      # OLED 0.42": anteprima carattere (I2C0)
└── UIDisplay.*           # OLED 128×64: boot, composizione, liste, info (I2C1)
docs/ui-prototype/        # prototipo HTML pixel-perfect della UI (emulatore)
tests/
├── run_tests.sh          # compila ed esegue i test nativi
├── test_t9.cpp
├── test_meshproto.cpp
└── shim/Arduino.h        # String & co. minimali per l'host
```

### Il protocollo in breve

Ogni frame sulla UART è `0x94 0xC3 <len MSB> <len LSB> <protobuf>`
(max 512 byte). All'avvio la tastiera invia `ToRadio{want_config_id}` e il
nodo risponde con la configurazione, `MyNodeInfo` (il nostro id), i
`Channel` configurati, `NodeInfo` per ogni nodo noto (nomi, SNR, batteria)
e infine `config_complete_id`. Da lì:

- **Invio**: `ToRadio{packet: MeshPacket{to, channel, decoded:
  Data{portnum: TEXT_MESSAGE_APP, payload}, id, want_ack}}`
- **Ricezione**: `FromRadio{packet}` con portnum `TEXT_MESSAGE_APP`
  (testo), `POSITION_APP` (coordinate sfixed32) o `TELEMETRY_APP`
  (batteria del nodo)
- **Esito**: `FromRadio{packet}` con portnum `ROUTING_APP` e
  `request_id` uguale all'id inviato (`error_reason` assente = consegnato)
- Un `ToRadio{heartbeat}` ogni 60 s tiene viva la sessione; se arriva
  `FromRadio{rebooted}` l'handshake riparte automaticamente.

### Cosa resta fuori (di proposito)

Amministrazione remota del nodo, modifica della configurazione, canali via
QR, trasferimento file e proxy MQTT sono compiti da app companion, non da
tastiera: il nodo si configura una volta con la CLI/app e la tastiera fa la
tastiera.

## Limiti noti e sviluppi futuri

- Il font grande dell'anteprima (`logisoso28`) potrebbe non coprire le
  accentate su alcune versioni di U8g2: in quel caso il firmware ripiega
  automaticamente su un font più piccolo (`10x20_te`).
- I messaggi diretti usano la cifratura del canale selezionato (il PKC dei
  DM è gestito dal nodo, se disponibile sul suo firmware).
- Idee: T9 predittivo con dizionario in flash, messaggi rapidi predefiniti,
  spegnimento display per risparmio energetico, buzzer di feedback.
