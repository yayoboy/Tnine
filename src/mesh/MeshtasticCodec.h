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
constexpr uint32_t MAX_CHANNELS = 8;

// PortNum
constexpr uint32_t PORT_TEXT_MESSAGE = 1;
constexpr uint32_t PORT_POSITION = 3;
constexpr uint32_t PORT_ROUTING = 5;
constexpr uint32_t PORT_TELEMETRY = 67;

// Routing.Error (motivi di NAK)
constexpr uint32_t ROUTING_ERROR_NONE = 0;

// Channel.Role
constexpr uint32_t CHANNEL_ROLE_DISABLED = 0;
constexpr uint32_t CHANNEL_ROLE_PRIMARY = 1;
constexpr uint32_t CHANNEL_ROLE_SECONDARY = 2;

// DeviceMetrics.battery_level > 100 significa alimentazione esterna.
constexpr uint32_t BATTERY_POWERED = 101;

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

// ToRadio{ heartbeat: {} } – mantiene viva la sessione client API.
size_t buildHeartbeatFrame(uint8_t* out, size_t cap);

// ---------------------------------------------------------------------------
// Parsing FromRadio
// ---------------------------------------------------------------------------

struct NodeInfoData {
    uint32_t num = 0;
    const char* shortName = nullptr;
    size_t shortLen = 0;
    const char* longName = nullptr;
    size_t longLen = 0;
    uint32_t lastHeard = 0;    // epoch secondi (0 = sconosciuto)
    float snr = 0.0f;
    int batteryLevel = -1;     // -1 = sconosciuto, BATTERY_POWERED = rete
};

struct FromRadioHandler {
    virtual void onMyNodeNum(uint32_t nodeNum) { (void)nodeNum; }
    virtual void onNodeInfo(const NodeInfoData& info) { (void)info; }
    virtual void onChannel(uint32_t index, const char* name, size_t nameLen,
                           uint32_t role) {
        (void)index; (void)name; (void)nameLen; (void)role;
    }
    virtual void onTextMessage(uint32_t fromNode, uint32_t channel,
                               const char* text, size_t len) {
        (void)fromNode; (void)channel; (void)text; (void)len;
    }
    // Coordinate in gradi * 1e7 (formato Meshtastic).
    virtual void onPosition(uint32_t fromNode, int32_t latitudeI,
                            int32_t longitudeI) {
        (void)fromNode; (void)latitudeI; (void)longitudeI;
    }
    // batteryLevel: 0-100, BATTERY_POWERED se alimentato da rete.
    virtual void onTelemetry(uint32_t fromNode, uint32_t batteryLevel) {
        (void)fromNode; (void)batteryLevel;
    }
    // ACK/NAK: requestId è l'id del MeshPacket inviato; error 0 = consegnato.
    virtual void onRoutingResult(uint32_t requestId, uint32_t error) {
        (void)requestId; (void)error;
    }
    virtual void onConfigComplete(uint32_t nonce) { (void)nonce; }
    // Il nodo si è riavviato: la sessione va rinegoziata.
    virtual void onRebooted() {}
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
