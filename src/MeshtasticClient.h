#pragma once

#include <Arduino.h>

#include "mesh/MeshtasticCodec.h"

// Client per il serial module di Meshtastic in modalità PROTO: parla il
// client API protobuf su UART1 (framing 0x94C3). Gestisce l'handshake di
// configurazione, l'heartbeat, il database dei nodi (per i nomi dei
// mittenti) e gli ACK di consegna.
class MeshtasticClient : private meshproto::FromRadioHandler {
public:
    enum class LinkState : uint8_t {
        Connecting,  // in attesa della risposta al want_config_id
        Connected,   // handshake completato
    };

    struct Callbacks {
        // Messaggio di testo ricevuto (senderName può essere "").
        void (*onTextMessage)(uint32_t fromNode, const char* senderName,
                              const String& text) = nullptr;
        // Esito di un invio: error 0 = consegnato, altrimenti vedi
        // meshproto::routingErrorLabel().
        void (*onSendResult)(uint32_t packetId, uint32_t error) = nullptr;
        void (*onLinkStateChange)(LinkState state) = nullptr;
    };

    void begin(const Callbacks& cb);
    void poll(uint32_t now);

    // Invia un messaggio (broadcast su MESH_CHANNEL). Ritorna l'id del
    // pacchetto per il matching dell'ACK, 0 su errore.
    uint32_t sendText(const String& msg);

    LinkState linkState() const { return _state; }
    uint32_t myNodeNum() const { return _myNodeNum; }

    // Nome breve di un nodo dal database ("" se sconosciuto).
    const char* shortNameOf(uint32_t nodeNum) const;

private:
    // meshproto::FromRadioHandler
    void onMyNodeNum(uint32_t nodeNum) override;
    void onNodeInfo(uint32_t nodeNum, const char* shortName, size_t shortLen,
                    const char* longName, size_t longLen) override;
    void onTextMessage(uint32_t fromNode, uint32_t channel,
                       const char* text, size_t len) override;
    void onRoutingResult(uint32_t requestId, uint32_t error) override;
    void onConfigComplete(uint32_t nonce) override;

    void setState(LinkState s);
    void sendFrame(const uint8_t* frame, size_t len);

    static constexpr uint8_t NODE_DB_SIZE = 32;
    static constexpr uint8_t SHORT_NAME_LEN = 8;  // 4 caratteri UTF-8 max

    struct NodeEntry {
        uint32_t num = 0;
        char shortName[SHORT_NAME_LEN + 1] = {0};
    };

    Callbacks _cb;
    LinkState _state = LinkState::Connecting;
    meshproto::FrameParser _parser;
    uint8_t _rxPayload[meshproto::MAX_PAYLOAD];

    uint32_t _myNodeNum = 0;
    uint32_t _configNonce = 0;
    uint32_t _nextPacketId = 1;
    uint32_t _lastConfigRequest = 0;
    uint32_t _lastHeartbeat = 0;
    uint32_t _lastNow = 0;

    NodeEntry _nodes[NODE_DB_SIZE];
    uint8_t _nodeWriteIdx = 0;
};
