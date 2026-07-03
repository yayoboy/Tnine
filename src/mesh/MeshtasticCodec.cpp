#include "MeshtasticCodec.h"

#include <string.h>

#include "ProtoReader.h"
#include "ProtoWriter.h"

namespace meshproto {

// Numeri di campo (da meshtastic/mesh.proto)
namespace fields {
// ToRadio
constexpr uint32_t TORADIO_PACKET = 1;
constexpr uint32_t TORADIO_WANT_CONFIG_ID = 3;
constexpr uint32_t TORADIO_HEARTBEAT = 7;
// FromRadio
constexpr uint32_t FROMRADIO_PACKET = 2;
constexpr uint32_t FROMRADIO_MY_INFO = 3;
constexpr uint32_t FROMRADIO_NODE_INFO = 4;
constexpr uint32_t FROMRADIO_CONFIG_COMPLETE_ID = 7;
// MeshPacket
constexpr uint32_t PACKET_FROM = 1;
constexpr uint32_t PACKET_TO = 2;
constexpr uint32_t PACKET_CHANNEL = 3;
constexpr uint32_t PACKET_DECODED = 4;
constexpr uint32_t PACKET_ID = 6;
constexpr uint32_t PACKET_WANT_ACK = 10;
// Data
constexpr uint32_t DATA_PORTNUM = 1;
constexpr uint32_t DATA_PAYLOAD = 2;
constexpr uint32_t DATA_REQUEST_ID = 6;
// MyNodeInfo
constexpr uint32_t MYINFO_MY_NODE_NUM = 1;
// NodeInfo
constexpr uint32_t NODEINFO_NUM = 1;
constexpr uint32_t NODEINFO_USER = 2;
// User
constexpr uint32_t USER_LONG_NAME = 2;
constexpr uint32_t USER_SHORT_NAME = 3;
// Routing
constexpr uint32_t ROUTING_ERROR_REASON = 3;
}  // namespace fields

// ---------------------------------------------------------------------------
// Framing
// ---------------------------------------------------------------------------

static size_t frame(uint8_t* out, size_t cap, const uint8_t* payload, size_t len) {
    if (len > MAX_PAYLOAD || cap < len + 4) return 0;
    out[0] = FRAME_START1;
    out[1] = FRAME_START2;
    out[2] = static_cast<uint8_t>(len >> 8);
    out[3] = static_cast<uint8_t>(len & 0xFF);
    memcpy(out + 4, payload, len);
    return len + 4;
}

// ---------------------------------------------------------------------------
// Encoding ToRadio
// ---------------------------------------------------------------------------

size_t buildTextMessageFrame(uint8_t* out, size_t cap,
                             uint32_t to, uint32_t channel, uint32_t packetId,
                             const char* text, size_t textLen) {
    // Data { portnum: TEXT_MESSAGE_APP, payload: text }
    uint8_t dataBuf[MAX_PAYLOAD];
    ProtoWriter data(dataBuf, sizeof(dataBuf));
    data.varintField(fields::DATA_PORTNUM, PORT_TEXT_MESSAGE);
    data.bytesField(fields::DATA_PAYLOAD,
                    reinterpret_cast<const uint8_t*>(text), textLen);
    if (!data.ok()) return 0;

    // MeshPacket { to, channel, decoded: Data, id, want_ack: true }
    uint8_t pktBuf[MAX_PAYLOAD];
    ProtoWriter pkt(pktBuf, sizeof(pktBuf));
    pkt.varintField(fields::PACKET_TO, to);
    if (channel != 0) pkt.varintField(fields::PACKET_CHANNEL, channel);
    pkt.bytesField(fields::PACKET_DECODED, dataBuf, data.size());
    pkt.varintField(fields::PACKET_ID, packetId);
    pkt.varintField(fields::PACKET_WANT_ACK, 1);
    if (!pkt.ok()) return 0;

    // ToRadio { packet: MeshPacket }
    uint8_t toRadioBuf[MAX_PAYLOAD];
    ProtoWriter toRadio(toRadioBuf, sizeof(toRadioBuf));
    toRadio.bytesField(fields::TORADIO_PACKET, pktBuf, pkt.size());
    if (!toRadio.ok()) return 0;

    return frame(out, cap, toRadioBuf, toRadio.size());
}

size_t buildWantConfigFrame(uint8_t* out, size_t cap, uint32_t nonce) {
    uint8_t buf[16];
    ProtoWriter toRadio(buf, sizeof(buf));
    toRadio.varintField(fields::TORADIO_WANT_CONFIG_ID, nonce);
    if (!toRadio.ok()) return 0;
    return frame(out, cap, buf, toRadio.size());
}

size_t buildHeartbeatFrame(uint8_t* out, size_t cap) {
    uint8_t buf[8];
    ProtoWriter toRadio(buf, sizeof(buf));
    toRadio.bytesField(fields::TORADIO_HEARTBEAT, nullptr, 0);  // Heartbeat {}
    if (!toRadio.ok()) return 0;
    return frame(out, cap, buf, toRadio.size());
}

// ---------------------------------------------------------------------------
// Parsing FromRadio
// ---------------------------------------------------------------------------

static void parseData(const uint8_t* buf, size_t len, uint32_t fromNode,
                      uint32_t channel, FromRadioHandler& handler) {
    uint32_t portnum = 0;
    const uint8_t* payload = nullptr;
    size_t payloadLen = 0;
    uint32_t requestId = 0;

    ProtoReader r(buf, len);
    while (r.next()) {
        switch (r.field()) {
            case fields::DATA_PORTNUM:
                portnum = static_cast<uint32_t>(r.varint());
                break;
            case fields::DATA_PAYLOAD:
                payload = r.data();
                payloadLen = r.dataLen();
                break;
            case fields::DATA_REQUEST_ID:
                requestId = static_cast<uint32_t>(r.varint());
                break;
        }
    }

    if (portnum == PORT_TEXT_MESSAGE && payload) {
        handler.onTextMessage(fromNode, channel,
                              reinterpret_cast<const char*>(payload), payloadLen);
    } else if (portnum == PORT_ROUTING && requestId != 0) {
        // Routing { error_reason } – assente in proto3 significa NONE (ACK).
        uint32_t error = ROUTING_ERROR_NONE;
        if (payload) {
            ProtoReader routing(payload, payloadLen);
            while (routing.next()) {
                if (routing.field() == fields::ROUTING_ERROR_REASON) {
                    error = static_cast<uint32_t>(routing.varint());
                }
            }
        }
        handler.onRoutingResult(requestId, error);
    }
}

static void parseMeshPacket(const uint8_t* buf, size_t len, FromRadioHandler& handler) {
    uint32_t from = 0;
    uint32_t channel = 0;
    const uint8_t* decoded = nullptr;
    size_t decodedLen = 0;

    ProtoReader r(buf, len);
    while (r.next()) {
        switch (r.field()) {
            case fields::PACKET_FROM:
                from = static_cast<uint32_t>(r.varint());
                break;
            case fields::PACKET_CHANNEL:
                channel = static_cast<uint32_t>(r.varint());
                break;
            case fields::PACKET_DECODED:
                decoded = r.data();
                decodedLen = r.dataLen();
                break;
        }
    }

    // I pacchetti che il nodo non ha potuto decifrare arrivano nel campo
    // `encrypted` e vengono ignorati.
    if (decoded) parseData(decoded, decodedLen, from, channel, handler);
}

static void parseNodeInfo(const uint8_t* buf, size_t len, FromRadioHandler& handler) {
    uint32_t num = 0;
    const char* shortName = nullptr;
    size_t shortLen = 0;
    const char* longName = nullptr;
    size_t longLen = 0;

    ProtoReader r(buf, len);
    while (r.next()) {
        if (r.field() == fields::NODEINFO_NUM) {
            num = static_cast<uint32_t>(r.varint());
        } else if (r.field() == fields::NODEINFO_USER && r.wireType() == 2) {
            ProtoReader user(r.data(), r.dataLen());
            while (user.next()) {
                if (user.field() == fields::USER_SHORT_NAME) {
                    shortName = reinterpret_cast<const char*>(user.data());
                    shortLen = user.dataLen();
                } else if (user.field() == fields::USER_LONG_NAME) {
                    longName = reinterpret_cast<const char*>(user.data());
                    longLen = user.dataLen();
                }
            }
        }
    }

    if (num != 0) {
        handler.onNodeInfo(num, shortName, shortLen, longName, longLen);
    }
}

void parseFromRadio(const uint8_t* payload, size_t len, FromRadioHandler& handler) {
    ProtoReader r(payload, len);
    while (r.next()) {
        switch (r.field()) {
            case fields::FROMRADIO_PACKET:
                if (r.wireType() == 2) parseMeshPacket(r.data(), r.dataLen(), handler);
                break;
            case fields::FROMRADIO_MY_INFO:
                if (r.wireType() == 2) {
                    ProtoReader info(r.data(), r.dataLen());
                    while (info.next()) {
                        if (info.field() == fields::MYINFO_MY_NODE_NUM) {
                            handler.onMyNodeNum(static_cast<uint32_t>(info.varint()));
                        }
                    }
                }
                break;
            case fields::FROMRADIO_NODE_INFO:
                if (r.wireType() == 2) parseNodeInfo(r.data(), r.dataLen(), handler);
                break;
            case fields::FROMRADIO_CONFIG_COMPLETE_ID:
                handler.onConfigComplete(static_cast<uint32_t>(r.varint()));
                break;
        }
    }
}

const char* routingErrorLabel(uint32_t error) {
    switch (error) {
        case 0:  return "ok";
        case 1:  return "no route";
        case 2:  return "nak";
        case 3:  return "timeout";
        case 5:  return "no canale";
        case 7:  return "no risp";
        case 9:  return "max ritx";
        default: return "err";
    }
}

// ---------------------------------------------------------------------------
// FrameParser
// ---------------------------------------------------------------------------

size_t FrameParser::feed(uint8_t b, uint8_t* payload) {
    switch (_state) {
        case Sync1:
            if (b == FRAME_START1) _state = Sync2;
            break;
        case Sync2:
            _state = (b == FRAME_START2) ? LenHi
                   : (b == FRAME_START1) ? Sync2
                                         : Sync1;
            break;
        case LenHi:
            _len = static_cast<size_t>(b) << 8;
            _state = LenLo;
            break;
        case LenLo:
            _len |= b;
            if (_len == 0 || _len > MAX_PAYLOAD) {
                _state = Sync1;  // lunghezza non valida: risincronizza
            } else {
                _pos = 0;
                _state = Body;
            }
            break;
        case Body:
            payload[_pos++] = b;
            if (_pos >= _len) {
                _state = Sync1;
                return _len;
            }
            break;
    }
    return 0;
}

}  // namespace meshproto
