#include "MeshtasticClient.h"

#include "config.h"

// Serial2 = UART1 sul core arduino-pico (earlephilhower).

// Copia una stringa UTF-8 troncandola a maxBytes senza spezzare un
// carattere multi-byte.
static void copyUtf8(char* dst, size_t maxBytes, const char* src, size_t srcLen) {
    size_t n = min(srcLen, maxBytes);
    // Non terminare in mezzo a un carattere: arretra oltre i byte di
    // continuazione (10xxxxxx) se il taglio ne separerebbe uno.
    while (n > 0 && n < srcLen &&
           (static_cast<uint8_t>(src[n]) & 0xC0) == 0x80) {
        --n;
    }
    if (src && n > 0) memcpy(dst, src, n);
    dst[n] = '\0';
}

void MeshtasticClient::begin(const Callbacks& cb) {
    _cb = cb;
    Serial2.setTX(PIN_MESH_TX);
    Serial2.setRX(PIN_MESH_RX);
    Serial2.begin(MESH_BAUD);

    // Nonce dell'handshake: basta che non sia 0 e vari tra i riavvii.
    _configNonce = micros() | 1;
    _nextPacketId = (_configNonce << 8) | 1;
}

void MeshtasticClient::setState(LinkState s) {
    if (s == _state) return;
    _state = s;
    if (_cb.onLinkStateChange) _cb.onLinkStateChange(s);
}

void MeshtasticClient::markDirty() {
    if (_cb.onStateDirty) _cb.onStateDirty();
}

void MeshtasticClient::sendFrame(const uint8_t* frame, size_t len) {
    Serial2.write(frame, len);
}

void MeshtasticClient::startHandshake(uint32_t now) {
    _lastConfigRequest = now;
    uint8_t frame[32];
    size_t n = meshproto::buildWantConfigFrame(frame, sizeof(frame), _configNonce);
    if (n) sendFrame(frame, n);
}

void MeshtasticClient::poll(uint32_t now) {
    _lastNow = now;

    // Handshake: richiedi la configurazione finché il nodo non risponde.
    if (_state == LinkState::Connecting &&
        (_lastConfigRequest == 0 || now - _lastConfigRequest >= MESH_CONFIG_RETRY_MS)) {
        startHandshake(now);
    }

    // Heartbeat per mantenere attiva la sessione client API.
    if (_state == LinkState::Connected && now - _lastHeartbeat >= MESH_HEARTBEAT_MS) {
        _lastHeartbeat = now;
        uint8_t frame[16];
        size_t n = meshproto::buildHeartbeatFrame(frame, sizeof(frame));
        if (n) sendFrame(frame, n);
    }

    while (Serial2.available() > 0) {
        uint8_t b = static_cast<uint8_t>(Serial2.read());
        size_t len = _parser.feed(b, _rxPayload);
        if (len > 0) {
            meshproto::parseFromRadio(_rxPayload, len, *this);
        }
    }
}

uint32_t MeshtasticClient::sendText(const String& msg, uint32_t to, uint32_t channel) {
    if (msg.length() == 0 || msg.length() > MESSAGE_MAX_LEN) return 0;

    uint32_t packetId = _nextPacketId++;
    if (_nextPacketId == 0) _nextPacketId = 1;

    uint8_t frame[meshproto::MAX_PAYLOAD + 4];
    size_t n = meshproto::buildTextMessageFrame(
        frame, sizeof(frame), to, channel, packetId, msg.c_str(), msg.length());
    if (n == 0) return 0;

    sendFrame(frame, n);
    return packetId;
}

const char* MeshtasticClient::shortNameOf(uint32_t nodeNum) const {
    for (uint8_t i = 0; i < _nodeCount; ++i) {
        if (_nodes[i].num == nodeNum) return _nodes[i].shortName;
    }
    return "";
}

// ---------------------------------------------------------------------------
// Eventi FromRadio
// ---------------------------------------------------------------------------

void MeshtasticClient::onMyNodeNum(uint32_t nodeNum) {
    _myNodeNum = nodeNum;
}

void MeshtasticClient::onNodeInfo(const meshproto::NodeInfoData& info) {
    // La batteria del nodo locale arriva con il suo NodeInfo all'handshake.
    if (info.num == _myNodeNum && info.batteryLevel >= 0) {
        _batteryLevel = info.batteryLevel;
    }
    if (info.num == _myNodeNum) {
        markDirty();
        return;  // il nodo locale non va nella lista destinatari
    }

    NodeEntry* slot = nullptr;
    for (uint8_t i = 0; i < _nodeCount; ++i) {
        if (_nodes[i].num == info.num) { slot = &_nodes[i]; break; }
    }
    if (!slot) {
        if (_nodeCount < NODE_DB_SIZE) {
            slot = &_nodes[_nodeCount++];
        } else {
            // Database pieno: sovrascrive a rotazione le voci più vecchie.
            slot = &_nodes[_nodeWriteIdx];
            _nodeWriteIdx = (_nodeWriteIdx + 1) % NODE_DB_SIZE;
        }
    }

    slot->num = info.num;
    copyUtf8(slot->shortName, SHORT_NAME_LEN, info.shortName, info.shortLen);
    copyUtf8(slot->longName, LONG_NAME_LEN, info.longName, info.longLen);
    if (info.lastHeard) slot->lastHeard = info.lastHeard;
    markDirty();
}

void MeshtasticClient::onChannel(uint32_t index, const char* name, size_t nameLen,
                                 uint32_t role) {
    if (index >= meshproto::MAX_CHANNELS) return;
    ChannelEntry& ch = _channels[index];
    ch.used = (role != meshproto::CHANNEL_ROLE_DISABLED);
    ch.role = static_cast<uint8_t>(role);
    copyUtf8(ch.name, CHANNEL_NAME_LEN, name, nameLen);
    markDirty();
}

void MeshtasticClient::onTextMessage(uint32_t fromNode, uint32_t channel,
                                     const char* text, size_t len) {
    if (fromNode == _myNodeNum) return;  // eco dei nostri stessi messaggi
    if (!_cb.onTextMessage) return;

    String msg;
    msg.reserve(len);
    for (size_t i = 0; i < len; ++i) msg += text[i];
    _cb.onTextMessage(fromNode, shortNameOf(fromNode), channel, msg);
}

void MeshtasticClient::onPosition(uint32_t fromNode, int32_t latitudeI,
                                  int32_t longitudeI) {
    if (fromNode == _myNodeNum) return;
    if (!_cb.onPosition) return;
    _cb.onPosition(fromNode, shortNameOf(fromNode), latitudeI, longitudeI);
}

void MeshtasticClient::onTelemetry(uint32_t fromNode, uint32_t batteryLevel) {
    // Interessa solo la telemetria del nodo collegato.
    if (fromNode == _myNodeNum || fromNode == 0) {
        _batteryLevel = static_cast<int>(batteryLevel);
        markDirty();
    }
}

void MeshtasticClient::onRoutingResult(uint32_t requestId, uint32_t error) {
    if (_cb.onSendResult) _cb.onSendResult(requestId, error);
}

void MeshtasticClient::onConfigComplete(uint32_t nonce) {
    if (nonce == _configNonce) {
        setState(LinkState::Connected);
        _lastHeartbeat = _lastNow;
    }
}

void MeshtasticClient::onRebooted() {
    // Il nodo si è riavviato: rinegozia subito la sessione.
    setState(LinkState::Connecting);
    startHandshake(_lastNow);
}
