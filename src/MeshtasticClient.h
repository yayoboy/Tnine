#pragma once

#include <Arduino.h>

#include "mesh/MeshtasticCodec.h"

// Client per il serial module di Meshtastic in modalità PROTO: parla il
// client API protobuf su UART1 (framing 0x94C3). Gestisce l'handshake di
// configurazione, l'heartbeat, il database dei nodi, la lista canali, la
// telemetria (batteria), il re-handshake dopo un riavvio del nodo e gli
// ACK di consegna.
class MeshtasticClient : private meshproto::FromRadioHandler {
public:
    enum class LinkState : uint8_t {
        Connecting,  // in attesa della risposta al want_config_id
        Connected,   // handshake completato
    };

    static constexpr uint8_t NODE_DB_SIZE = 32;
    static constexpr uint8_t SHORT_NAME_LEN = 8;   // 4 caratteri UTF-8 max
    static constexpr uint8_t LONG_NAME_LEN = 20;
    static constexpr uint8_t CHANNEL_NAME_LEN = 12;

    struct NodeEntry {
        uint32_t num = 0;
        char shortName[SHORT_NAME_LEN + 1] = {0};
        char longName[LONG_NAME_LEN + 1] = {0};
        uint32_t lastHeard = 0;   // epoch secondi (0 = sconosciuto)
    };

    struct ChannelEntry {
        bool used = false;
        uint8_t role = 0;         // meshproto::CHANNEL_ROLE_*
        char name[CHANNEL_NAME_LEN + 1] = {0};
    };

    struct Callbacks {
        // Messaggio di testo ricevuto (senderName può essere "").
        void (*onTextMessage)(uint32_t fromNode, const char* senderName,
                              uint32_t channel, const String& text) = nullptr;
        // Posizione ricevuta (gradi * 1e7).
        void (*onPosition)(uint32_t fromNode, const char* senderName,
                           int32_t latitudeI, int32_t longitudeI) = nullptr;
        // Esito di un invio: error 0 = consegnato, altrimenti vedi
        // meshproto::routingErrorLabel().
        void (*onSendResult)(uint32_t packetId, uint32_t error) = nullptr;
        void (*onLinkStateChange)(LinkState state) = nullptr;
        // Stato interno cambiato (nodi, canali, batteria): la UI va ridisegnata.
        void (*onStateDirty)() = nullptr;
    };

    void begin(const Callbacks& cb);
    void poll(uint32_t now);

    // Invia un messaggio a `to` (BROADCAST_ADDR per tutti) sul canale
    // `channel`. Ritorna l'id del pacchetto per il matching dell'ACK, 0 su
    // errore.
    uint32_t sendText(const String& msg, uint32_t to, uint32_t channel);

    LinkState linkState() const { return _state; }
    uint32_t myNodeNum() const { return _myNodeNum; }

    // Batteria del nodo collegato: -1 sconosciuta, 0-100 percentuale,
    // meshproto::BATTERY_POWERED se alimentato da rete.
    int batteryLevel() const { return _batteryLevel; }

    // Database nodi (esclude il nodo locale).
    uint8_t nodeCount() const { return _nodeCount; }
    const NodeEntry& node(uint8_t idx) const { return _nodes[idx]; }
    const char* shortNameOf(uint32_t nodeNum) const;

    // Canali configurati sul nodo.
    const ChannelEntry& channel(uint8_t idx) const { return _channels[idx]; }

private:
    // meshproto::FromRadioHandler
    void onMyNodeNum(uint32_t nodeNum) override;
    void onNodeInfo(const meshproto::NodeInfoData& info) override;
    void onChannel(uint32_t index, const char* name, size_t nameLen,
                   uint32_t role) override;
    void onTextMessage(uint32_t fromNode, uint32_t channel,
                       const char* text, size_t len) override;
    void onPosition(uint32_t fromNode, int32_t latitudeI,
                    int32_t longitudeI) override;
    void onTelemetry(uint32_t fromNode, uint32_t batteryLevel) override;
    void onRoutingResult(uint32_t requestId, uint32_t error) override;
    void onConfigComplete(uint32_t nonce) override;
    void onRebooted() override;

    void setState(LinkState s);
    void markDirty();
    void sendFrame(const uint8_t* frame, size_t len);
    void startHandshake(uint32_t now);

    Callbacks _cb;
    LinkState _state = LinkState::Connecting;
    meshproto::FrameParser _parser;
    uint8_t _rxPayload[meshproto::MAX_PAYLOAD];

    uint32_t _myNodeNum = 0;
    int _batteryLevel = -1;
    uint32_t _configNonce = 0;
    uint32_t _nextPacketId = 1;
    uint32_t _lastConfigRequest = 0;
    uint32_t _lastHeartbeat = 0;
    uint32_t _lastNow = 0;

    NodeEntry _nodes[NODE_DB_SIZE];
    uint8_t _nodeCount = 0;
    uint8_t _nodeWriteIdx = 0;
    ChannelEntry _channels[meshproto::MAX_CHANNELS];
};
