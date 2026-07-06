// Tnine – tastiera T9 per Meshtastic su RP2040 (UI "Retro '84")
//
// Composizione:
//   0-9        input T9 multi-tap (in modalità 123 inserisce la cifra)
//   * breve    backspace (o annulla il carattere candidato)
//   * lungo    cancella tutto il messaggio
//   # breve    invia il messaggio (al destinatario/canale selezionati)
//   # lungo    cambia modalità (abc -> ABC -> 123)
//   0 lungo    apre il menu
//
// Menu e liste:
//   2/8        su/giù (nel dettaglio: scorri il testo)
//   4/6        (nel dettaglio) cambia pulsante
//   5 o #      seleziona / attiva il pulsante
//   *          indietro

#include <Arduino.h>

#include "MatrixKeypad.h"
#include "MeshtasticClient.h"
#include "PreviewDisplay.h"
#include "T9Engine.h"
#include "UIDisplay.h"
#include "config.h"

static MatrixKeypad keypad;
static T9Engine t9;
static MeshtasticClient mesh;
static PreviewDisplay preview;
static UIDisplay ui;

// --- Stato UI -----------------------------------------------------------------

enum class Screen : uint8_t {
    Boot,
    Compose,
    Menu,
    PickDest,
    PickChannel,
    History,
    HistoryDetail,
    QuickMsgs,
    Info,
};

static Screen screen = Screen::Boot;
static String status;
static uint32_t statusUntil = 0;
static bool linkConnected = false;
static uint32_t pendingPacketId = 0;
static bool dirty = true;
static uint32_t lastBootFrame = 0;

// Destinazione e canale correnti.
static uint32_t destNode = meshproto::BROADCAST_ADDR;
static String destLabel = "Tutti";
static uint32_t selChannel = MESH_CHANNEL;

// Messaggi rapidi (dal prototipo UI).
static const char* const QUICK_MSGS[] = {
    "Ok", "Si", "No", "Arrivo tra poco", "Posizione?", "Aiuto! SOS",
};
static constexpr int QUICK_COUNT = sizeof(QUICK_MSGS) / sizeof(QUICK_MSGS[0]);

// Storico messaggi ricevuti (il più recente in testa).
struct HistoryEntry {
    uint32_t fromNode = 0;
    String sender;
    String text;
    uint8_t channel = 0;
};
static constexpr uint8_t HISTORY_SIZE = 8;
static HistoryEntry history[HISTORY_SIZE];
static uint8_t historyCount = 0;
static uint8_t unreadCount = 0;

// Liste e selezioni.
static constexpr int MAX_LIST_ITEMS = MeshtasticClient::NODE_DB_SIZE + 1;
static String listItems[MAX_LIST_ITEMS];
static int8_t listBars[MAX_LIST_ITEMS];
static bool listHasBars = false;
static int listCount = 0;
static int listSel = 0;
// Mappa voce lista -> indice reale (nodo, canale, voce storico...).
static uint32_t listRef[MAX_LIST_ITEMS];
static int menuSel = 0;
static int detailScroll = 0;
static int detailBtn = 0;
static int detailFrom = 0;  // voce di History aperta nel dettaglio

static void setStatus(const char* msg, uint32_t durationMs = 3000) {
    status = msg;
    statusUntil = millis() + durationMs;
    dirty = true;
}

// --- Formattazione -------------------------------------------------------------

static String nodeLabel(uint32_t nodeNum, const char* senderName) {
    if (senderName && senderName[0] != '\0') return String(senderName);
    char hex[12];
    snprintf(hex, sizeof(hex), "%08lx", static_cast<unsigned long>(nodeNum));
    return String(hex + 4);  // ultime 4 cifre esadecimali dell'id nodo
}

// Coordinate Meshtastic (gradi * 1e7) -> "45.06890" (5 decimali).
static String formatCoord(int32_t coordI) {
    char buf[16];
    int32_t whole = coordI / 10000000;
    uint32_t frac = (coordI < 0 ? -(coordI % 10000000) : coordI % 10000000) / 100;
    snprintf(buf, sizeof(buf), "%s%ld.%05lu",
             (coordI < 0 && whole == 0) ? "-" : "",
             static_cast<long>(whole), static_cast<unsigned long>(frac));
    return String(buf);
}

static String channelLabel(uint8_t idx) {
    const MeshtasticClient::ChannelEntry& ch = mesh.channel(idx);
    if (ch.name[0] != '\0') return String(ch.name);
    if (ch.role == meshproto::CHANNEL_ROLE_PRIMARY) return String("Primario");
    return String("Canale ") + String(idx);
}

// SNR in dB -> 0..4 tacche di segnale.
static int8_t snrToBars(int8_t snrDb) {
    if (snrDb == INT8_MIN) return -1;
    if (snrDb >= 5) return 4;
    if (snrDb >= 0) return 3;
    if (snrDb >= -7) return 2;
    if (snrDb >= -15) return 1;
    return 0;
}

// --- Storico -------------------------------------------------------------------

static void pushHistory(uint32_t fromNode, const String& sender,
                        const String& text, uint8_t channel) {
    for (int i = HISTORY_SIZE - 1; i > 0; --i) history[i] = history[i - 1];
    history[0].fromNode = fromNode;
    history[0].sender = sender;
    history[0].text = text;
    history[0].channel = channel;
    if (historyCount < HISTORY_SIZE) ++historyCount;
    if (unreadCount < 99) ++unreadCount;
    dirty = true;
}

// --- Callback dal client Meshtastic ---------------------------------------------

static void onTextMessage(uint32_t fromNode, const char* senderName,
                          uint32_t channel, const String& text) {
    pushHistory(fromNode, nodeLabel(fromNode, senderName), text,
                static_cast<uint8_t>(channel));
}

static void onPosition(uint32_t fromNode, const char* senderName,
                       int32_t latitudeI, int32_t longitudeI) {
    String text = "pos " + formatCoord(latitudeI) + "," + formatCoord(longitudeI);
    pushHistory(fromNode, nodeLabel(fromNode, senderName), text, 0);
}

static void onSendResult(uint32_t packetId, uint32_t error) {
    if (packetId != pendingPacketId) return;
    pendingPacketId = 0;
    if (error == meshproto::ROUTING_ERROR_NONE) {
        setStatus("CONSEGNATO");
    } else {
        setStatus(meshproto::routingErrorLabel(error));
    }
}

static void onLinkStateChange(MeshtasticClient::LinkState state) {
    linkConnected = (state == MeshtasticClient::LinkState::Connected);
    if (linkConnected && screen == Screen::Boot) screen = Screen::Compose;
    dirty = true;
}

static void onStateDirty() {
    dirty = true;
}

// --- Costruzione liste -----------------------------------------------------------

static void openMenu() {
    screen = Screen::Menu;
    menuSel = 0;
    dirty = true;
}

static void buildDestList() {
    listCount = 0;
    listHasBars = true;
    listItems[listCount] = "Tutti (broadcast)";
    listBars[listCount] = -1;
    listRef[listCount++] = meshproto::BROADCAST_ADDR;
    for (uint8_t i = 0; i < mesh.nodeCount() && listCount < MAX_LIST_ITEMS; ++i) {
        const MeshtasticClient::NodeEntry& n = mesh.node(i);
        String label = nodeLabel(n.num, n.shortName);
        if (n.longName[0] != '\0') label += String(" ") + n.longName;
        listItems[listCount] = label;
        listBars[listCount] = snrToBars(n.snrDb);
        listRef[listCount++] = n.num;
    }
    listSel = 0;
    for (int i = 0; i < listCount; ++i) {
        if (listRef[i] == destNode) { listSel = i; break; }
    }
}

static void buildChannelList() {
    listCount = 0;
    listHasBars = false;
    for (uint8_t i = 0; i < meshproto::MAX_CHANNELS; ++i) {
        if (!mesh.channel(i).used) continue;
        listItems[listCount] = channelLabel(i);
        listRef[listCount++] = i;
    }
    if (listCount == 0) {  // nessuna lista ricevuta: almeno il primario
        listItems[0] = "Primario";
        listRef[0] = 0;
        listCount = 1;
    }
    listSel = 0;
    for (int i = 0; i < listCount; ++i) {
        if (listRef[i] == selChannel) { listSel = i; break; }
    }
}

static void buildHistoryList() {
    listCount = 0;
    listHasBars = false;
    for (uint8_t i = 0; i < historyCount && listCount < MAX_LIST_ITEMS; ++i) {
        listItems[listCount] = history[i].sender + ": " + history[i].text;
        listRef[listCount++] = i;
    }
    listSel = 0;
}

static void buildQuickList() {
    listCount = 0;
    listHasBars = false;
    for (int i = 0; i < QUICK_COUNT; ++i) {
        listItems[listCount] = QUICK_MSGS[i];
        listRef[listCount++] = i;
    }
    listSel = 0;
}

static String myNodeId() {
    char hex[12];
    snprintf(hex, sizeof(hex), "!%08lx", static_cast<unsigned long>(mesh.myNodeNum()));
    return String(hex);
}

// --- Invio -----------------------------------------------------------------------

static void sendMessage() {
    t9.commitPending();
    if (t9.text().length() == 0) {
        setStatus("VUOTO");
        return;
    }
    uint32_t id = mesh.sendText(t9.text(), destNode, selChannel);
    if (id != 0) {
        pendingPacketId = id;
        t9.clear();
        setStatus("INVIO...", 30000);  // sostituito dall'esito dell'ACK
    } else {
        setStatus("ERRORE");
    }
}

// --- Gestione tasti ----------------------------------------------------------------

static void handleComposeKey(const KeyEvent& ev, uint32_t now) {
    if (ev.key >= '0' && ev.key <= '9') {
        if (ev.type == KeyEvent::Down) {
            dirty |= t9.handleKey(ev.key, now);
        } else if (ev.key == '0' && ev.type == KeyEvent::LongHold) {
            t9.backspace();  // annulla lo spazio candidato del Down
            openMenu();
        }
        return;
    }

    if (ev.key == '*') {
        if (ev.type == KeyEvent::Up && !ev.wasLong) {
            dirty |= t9.backspace();
        } else if (ev.type == KeyEvent::LongHold) {
            t9.clear();
            setStatus("CANCELLATO");
        }
        return;
    }

    if (ev.key == '#') {
        if (ev.type == KeyEvent::Up && !ev.wasLong) {
            sendMessage();
        } else if (ev.type == KeyEvent::LongHold) {
            t9.cycleMode();
            dirty = true;
        }
        return;
    }
}

// Navigazione comune di menu/liste: ritorna -1 nessuna azione, 0 indietro,
// 1 selezione.
static int listNav(const KeyEvent& ev) {
    if (ev.type == KeyEvent::Down) {
        if (ev.key == '2' && listSel > 0) { --listSel; dirty = true; }
        if (ev.key == '8' && listSel < listCount - 1) { ++listSel; dirty = true; }
    }
    if (ev.type == KeyEvent::Up && !ev.wasLong) {
        if (ev.key == '*') return 0;
        if (ev.key == '#' || ev.key == '5') return 1;
    }
    return -1;
}

static const char* const MENU_ITEMS[] = {
    "Destinatario", "Canale", "Messaggi", "Msg rapidi", "Info",
};
static constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

static void handleMenuKey(const KeyEvent& ev) {
    // Il menu riusa la navigazione delle liste su menuSel.
    listCount = MENU_COUNT;
    int prevSel = listSel;
    listSel = menuSel;
    int action = listNav(ev);
    menuSel = listSel;
    listSel = prevSel;

    if (action == 0) {
        screen = Screen::Compose;
        dirty = true;
    } else if (action == 1) {
        switch (menuSel) {
            case 0: buildDestList();    screen = Screen::PickDest;    break;
            case 1: buildChannelList(); screen = Screen::PickChannel; break;
            case 2:
                buildHistoryList();
                unreadCount = 0;
                screen = Screen::History;
                break;
            case 3: buildQuickList();   screen = Screen::QuickMsgs;   break;
            case 4: screen = Screen::Info;                            break;
        }
        dirty = true;
    }
}

static void handleListKey(const KeyEvent& ev) {
    int action = listNav(ev);
    if (action < 0) return;

    if (action == 0) {  // indietro
        screen = Screen::Menu;
        dirty = true;
        return;
    }

    // Selezione
    switch (screen) {
        case Screen::PickDest:
            destNode = listRef[listSel];
            destLabel = (destNode == meshproto::BROADCAST_ADDR)
                            ? String("Tutti")
                            : nodeLabel(destNode, mesh.shortNameOf(destNode));
            setStatus("DEST OK");
            screen = Screen::Compose;
            break;
        case Screen::PickChannel:
            selChannel = listRef[listSel];
            setStatus("CANALE OK");
            screen = Screen::Compose;
            break;
        case Screen::History:
            if (listCount > 0) {
                detailFrom = listRef[listSel];
                detailScroll = 0;
                detailBtn = 0;
                screen = Screen::HistoryDetail;
            }
            break;
        case Screen::QuickMsgs:
            t9.setText(QUICK_MSGS[listRef[listSel]]);
            setStatus("PRONTO: # INVIA");
            screen = Screen::Compose;
            break;
        default:
            break;
    }
    dirty = true;
}

static void replyToDetail() {
    const HistoryEntry& h = history[detailFrom];
    if (h.fromNode != 0) {
        destNode = h.fromNode;
        destLabel = h.sender;
        setStatus("DEST OK");
    }
    screen = Screen::Compose;
    dirty = true;
}

static void handleHistoryDetailKey(const KeyEvent& ev, const String& text) {
    if (ev.type == KeyEvent::Down) {
        int total = UIDisplay::detailLines(text);
        if (ev.key == '2' && detailScroll > 0) { --detailScroll; dirty = true; }
        if (ev.key == '8' && detailScroll < total - 1) { ++detailScroll; dirty = true; }
        if (ev.key == '4' && detailBtn > 0) { --detailBtn; dirty = true; }
        if (ev.key == '6' && detailBtn < 1) { ++detailBtn; dirty = true; }
    }
    if (ev.type == KeyEvent::Up && !ev.wasLong) {
        if (ev.key == '*') {
            screen = Screen::History;
            buildHistoryList();
            dirty = true;
        } else if (ev.key == '#' || ev.key == '5') {
            if (detailBtn == 0) {
                replyToDetail();
            } else {
                screen = Screen::History;
                buildHistoryList();
                dirty = true;
            }
        }
    }
}

static void handleInfoKey(const KeyEvent& ev) {
    if (ev.type == KeyEvent::Up && !ev.wasLong &&
        (ev.key == '*' || ev.key == '#' || ev.key == '5')) {
        screen = Screen::Menu;
        dirty = true;
    }
}

static void handleKeyEvent(const KeyEvent& ev, uint32_t now) {
    switch (screen) {
        case Screen::Boot:
            // Un tasto qualsiasi salta alla composizione.
            if (ev.type == KeyEvent::Down) {
                screen = Screen::Compose;
                dirty = true;
            }
            break;
        case Screen::Compose:
            handleComposeKey(ev, now);
            break;
        case Screen::Menu:
            handleMenuKey(ev);
            break;
        case Screen::PickDest:
        case Screen::PickChannel:
        case Screen::History:
        case Screen::QuickMsgs:
            handleListKey(ev);
            break;
        case Screen::HistoryDetail:
            handleHistoryDetailKey(ev, history[detailFrom].sender + ": " +
                                           history[detailFrom].text);
            break;
        case Screen::Info:
            handleInfoKey(ev);
            break;
    }
}

// --- Rendering ---------------------------------------------------------------------

static void render() {
    preview.render(t9);

    switch (screen) {
        case Screen::Boot: {
            // Gauge animato finché il nodo non risponde all'handshake.
            int pct = static_cast<int>((millis() / 60) % 100);
            ui.renderBoot(pct);
            break;
        }
        case Screen::Compose: {
            String right;
            if (status.length() > 0) {
                right = status;
            } else if (!linkConnected) {
                right = "MESH...";
            } else {
                right = ">" + destLabel;
            }
            ui.renderCompose(t9, right, mesh.batteryLevel(), unreadCount);
            break;
        }
        case Screen::Menu: {
            for (int i = 0; i < MENU_COUNT; ++i) listItems[i] = MENU_ITEMS[i];
            ui.renderList("MENU", myNodeId(), listItems, MENU_COUNT, menuSel);
            break;
        }
        case Screen::PickDest:
            ui.renderList("DESTINATARIO", String(listCount - 1) + " NODI",
                          listItems, listCount, listSel,
                          listHasBars ? listBars : nullptr);
            break;
        case Screen::PickChannel:
            ui.renderList("CANALE", "", listItems, listCount, listSel);
            break;
        case Screen::History:
            if (listCount == 0) {
                listItems[0] = "(nessun messaggio)";
                ui.renderList("MESSAGGI", "0", listItems, 1, 0);
            } else {
                ui.renderList("MESSAGGI", String(listCount), listItems,
                              listCount, listSel);
            }
            break;
        case Screen::HistoryDetail:
            ui.renderDetail("MESSAGGIO", history[detailFrom].sender,
                            history[detailFrom].sender + ": " +
                                history[detailFrom].text,
                            detailScroll, "RISPONDI", "CHIUDI", detailBtn);
            break;
        case Screen::QuickMsgs:
            ui.renderList("MSG RAPIDI", "", listItems, listCount, listSel);
            break;
        case Screen::Info:
            ui.renderInfo(myNodeId(), mesh.batteryLevel(), linkConnected,
                          mesh.nodeCount(), destLabel, channelLabel(selChannel));
            break;
    }
}

void setup() {
    Serial.begin(115200);  // log di debug via USB

    keypad.begin();

    MeshtasticClient::Callbacks cb;
    cb.onTextMessage = onTextMessage;
    cb.onPosition = onPosition;
    cb.onSendResult = onSendResult;
    cb.onLinkStateChange = onLinkStateChange;
    cb.onStateDirty = onStateDirty;
    mesh.begin(cb);

    preview.begin();
    ui.begin();
}

void loop() {
    uint32_t now = millis();

    mesh.poll(now);

    KeyEvent ev = keypad.poll(now);
    if (ev.type != KeyEvent::None) {
        handleKeyEvent(ev, now);
    }

    dirty |= t9.update(now);

    if (status.length() > 0 && static_cast<int32_t>(now - statusUntil) >= 0) {
        status = "";
        dirty = true;
    }

    // Il boot si ridisegna periodicamente per animare il gauge.
    if (screen == Screen::Boot && now - lastBootFrame >= 100) {
        lastBootFrame = now;
        dirty = true;
    }

    if (dirty) {
        dirty = false;
        render();
    }
}
