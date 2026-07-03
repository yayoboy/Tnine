#include "MeshtasticClient.h"

#include "config.h"

// Serial2 = UART1 sul core arduino-pico (earlephilhower).

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

void MeshtasticClient::sendFrame(const uint8_t* frame, size_t len) {
    Serial2.write(frame, len);
}

void MeshtasticClient::poll(uint32_t now) {
    _lastNow = now;

    // Handshake: richiedi la configurazione finché il nodo non risponde.
    if (_state == LinkState::Connecting &&
        (now - _lastConfigRequest >= MESH_CONFIG_RETRY_MS || _lastConfigRequest == 0)) {
        _lastConfigRequest = now;
        uint8_t frame[32];
        size_t n = meshproto::buildWantConfigFrame(frame, sizeof(frame), _configNonce);
        if (n) sendFrame(frame, n);
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

uint32_t MeshtasticClient::sendText(const String& msg) {
    if (msg.length() == 0 || msg.length() > MESSAGE_MAX_LEN) return 0;

    uint32_t packetId = _nextPacketId++;
    if (_nextPacketId == 0) _nextPacketId = 1;

    uint8_t frame[meshproto::MAX_PAYLOAD + 4];
    size_t n = meshproto::buildTextMessageFrame(
        frame, sizeof(frame), meshproto::BROADCAST_ADDR, MESH_CHANNEL,
        packetId, msg.c_str(), msg.length());
    if (n == 0) return 0;

    sendFrame(frame, n);
    return packetId;
}

const char* MeshtasticClient::shortNameOf(uint32_t nodeNum) const {
    for (const NodeEntry& e : _nodes) {
        if (e.num == nodeNum) return e.shortName;
    }
    return "";
}

// ---------------------------------------------------------------------------
// Eventi FromRadio
// ---------------------------------------------------------------------------

void MeshtasticClient::onMyNodeNum(uint32_t nodeNum) {
    _myNodeNum = nodeNum;
}

void MeshtasticClient::onNodeInfo(uint32_t nodeNum,
                                  const char* shortName, size_t shortLen,
                                  const char* longName, size_t longLen) {
    (void)longName;
    (void)longLen;

    NodeEntry* slot = nullptr;
    for (NodeEntry& e : _nodes) {
        if (e.num == nodeNum) { slot = &e; break; }
    }
    if (!slot) {
        slot = &_nodes[_nodeWriteIdx];
        _nodeWriteIdx = (_nodeWriteIdx + 1) % NODE_DB_SIZE;
    }

    slot->num = nodeNum;
    size_t n = min(shortLen, static_cast<size_t>(SHORT_NAME_LEN));
    if (shortName && n > 0) memcpy(slot->shortName, shortName, n);
    slot->shortName[n] = '\0';
}

void MeshtasticClient::onTextMessage(uint32_t fromNode, uint32_t channel,
                                     const char* text, size_t len) {
    (void)channel;
    if (fromNode == _myNodeNum) return;  // eco dei nostri stessi messaggi
    if (!_cb.onTextMessage) return;

    String msg;
    msg.reserve(len);
    for (size_t i = 0; i < len; ++i) msg += text[i];
    _cb.onTextMessage(fromNode, shortNameOf(fromNode), msg);
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
