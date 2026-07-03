#pragma once

#include <stddef.h>
#include <stdint.h>

// Subset del client API di Meshtastic usato dalla tastiera, parlato dal
// serial module in modalità PROTO. Ogni frame è:
//
//   0x94 0xC3 <len MSB> <len LSB> <protobuf ToRadio/FromRadio>
//
// Implementazione protobuf fatta a mano (solo i campi che servono), così da
// non dipendere da nanopb e poter testare tutto in nativo.
// Riferimento: https://meshtastic.org/docs/development/reference/protobufs
namespace meshproto {

constexpr uint8_t FRAME_START1 = 0x94;
constexpr uint8_t FRAME_START2 = 0xC3;
constexpr size_t MAX_PAYLOAD = 512;   // MTU del client API

constexpr uint32_t BROADCAST_ADDR = 0xFFFFFFFFu;

// PortNum
constexpr uint32_t PORT_TEXT_MESSAGE = 1;
constexpr uint32_t PORT_ROUTING = 5;

// Routing.Error (motivi di NAK)
constexpr uint32_t ROUTING_ERROR_NONE = 0;

// ---------------------------------------------------------------------------
// Costruzione frame ToRadio (ritornano la lunghezza totale, 0 su errore)
// ---------------------------------------------------------------------------

// ToRadio{ packet: MeshPacket{ to, channel, id, want_ack,
//          decoded: Data{ portnum: TEXT_MESSAGE_APP, payload } } }
size_t buildTextMessageFrame(uint8_t* out, size_t cap,
                             uint32_t to, uint32_t channel, uint32_t packetId,
                             const char* text, size_t textLen);

// ToRadio{ want_config_id: nonce } – avvia l'handshake di configurazione.
size_t buildWantConfigFrame(uint8_t* out, size_t cap, uint32_t nonce);

// ToRadio{ heartbeat: {} } – mantiene viva la connessione client API.
size_t buildHeartbeatFrame(uint8_t* out, size_t cap);

// ---------------------------------------------------------------------------
// Parsing FromRadio
// ---------------------------------------------------------------------------

struct FromRadioHandler {
    virtual void onMyNodeNum(uint32_t nodeNum) { (void)nodeNum; }
    virtual void onNodeInfo(uint32_t nodeNum,
                            const char* shortName, size_t shortLen,
                            const char* longName, size_t longLen) {
        (void)nodeNum; (void)shortName; (void)shortLen; (void)longName; (void)longLen;
    }
    virtual void onTextMessage(uint32_t fromNode, uint32_t channel,
                               const char* text, size_t len) {
        (void)fromNode; (void)channel; (void)text; (void)len;
    }
    // ACK/NAK: requestId è l'id del MeshPacket inviato; error 0 = consegnato.
    virtual void onRoutingResult(uint32_t requestId, uint32_t error) {
        (void)requestId; (void)error;
    }
    virtual void onConfigComplete(uint32_t nonce) { (void)nonce; }
    virtual ~FromRadioHandler() = default;
};

void parseFromRadio(const uint8_t* payload, size_t len, FromRadioHandler& handler);

// Descrizione breve di un Routing.Error.
const char* routingErrorLabel(uint32_t error);

// ---------------------------------------------------------------------------
// Deframer dello stream seriale (si risincronizza sui byte magici)
// ---------------------------------------------------------------------------

class FrameParser {
public:
    // Elabora un byte; quando un frame è completo ritorna la lunghezza del
    // payload copiato in `payload` (capienza >= MAX_PAYLOAD), altrimenti 0.
    size_t feed(uint8_t b, uint8_t* payload);

private:
    enum State : uint8_t { Sync1, Sync2, LenHi, LenLo, Body };
    State _state = Sync1;
    size_t _len = 0;
    size_t _pos = 0;
};

}  // namespace meshproto
