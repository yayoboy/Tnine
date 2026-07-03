// Test nativi del codec Meshtastic: protobuf, framing e parsing FromRadio.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mesh/MeshtasticCodec.h"
#include "mesh/ProtoReader.h"
#include "mesh/ProtoWriter.h"

using namespace meshproto;

// ---------------------------------------------------------------------------
// Roundtrip ProtoWriter -> ProtoReader
// ---------------------------------------------------------------------------
static void testProtoRoundtrip() {
    uint8_t buf[64];
    ProtoWriter w(buf, sizeof(buf));
    w.varintField(1, 42);
    w.varintField(2, 0xFFFFFFFFu);   // valore a 32 bit pieno
    w.bytesField(3, reinterpret_cast<const uint8_t*>("ciao"), 4);
    assert(w.ok());

    ProtoReader r(buf, w.size());
    assert(r.next() && r.field() == 1 && r.varint() == 42);
    assert(r.next() && r.field() == 2 && r.varint() == 0xFFFFFFFFu);
    assert(r.next() && r.field() == 3 && r.dataLen() == 4);
    assert(memcmp(r.data(), "ciao", 4) == 0);
    assert(!r.next());

    // Overflow rilevato
    uint8_t tiny[4];
    ProtoWriter small(tiny, sizeof(tiny));
    small.bytesField(1, reinterpret_cast<const uint8_t*>("troppo lungo"), 12);
    assert(!small.ok());

    // Dati troncati: il reader si ferma senza leggere oltre
    ProtoReader bad(buf, 1);
    assert(!bad.next() || bad.field() != 0);

    printf("  roundtrip protobuf: ok\n");
}

// ---------------------------------------------------------------------------
// buildTextMessageFrame: verifica la struttura ToRadio completa
// ---------------------------------------------------------------------------
static void testBuildTextMessage() {
    uint8_t frame[MAX_PAYLOAD + 4];
    size_t n = buildTextMessageFrame(frame, sizeof(frame),
                                     BROADCAST_ADDR, 0, 0xABCD, "ciao à", 7);
    assert(n > 4);
    assert(frame[0] == FRAME_START1 && frame[1] == FRAME_START2);
    size_t payloadLen = (static_cast<size_t>(frame[2]) << 8) | frame[3];
    assert(payloadLen == n - 4);

    // ToRadio { 1: MeshPacket }
    ProtoReader toRadio(frame + 4, payloadLen);
    assert(toRadio.next() && toRadio.field() == 1 && toRadio.wireType() == 2);

    // MeshPacket { to, decoded, id, want_ack }
    uint32_t to = 0, id = 0, wantAck = 0;
    const uint8_t* decoded = nullptr;
    size_t decodedLen = 0;
    ProtoReader pkt(toRadio.data(), toRadio.dataLen());
    while (pkt.next()) {
        switch (pkt.field()) {
            case 2:  to = pkt.varint(); break;
            case 4:  decoded = pkt.data(); decodedLen = pkt.dataLen(); break;
            case 6:  id = pkt.varint(); break;
            case 10: wantAck = pkt.varint(); break;
        }
    }
    assert(to == BROADCAST_ADDR);
    assert(id == 0xABCD);
    assert(wantAck == 1);
    assert(decoded);

    // Data { portnum: TEXT, payload }
    uint32_t portnum = 0;
    std::string text;
    ProtoReader data(decoded, decodedLen);
    while (data.next()) {
        if (data.field() == 1) portnum = data.varint();
        if (data.field() == 2) text.assign(reinterpret_cast<const char*>(data.data()), data.dataLen());
    }
    assert(portnum == PORT_TEXT_MESSAGE);
    assert(text == "ciao à");

    // Messaggio troppo lungo per il buffer -> 0
    char big[600];
    memset(big, 'x', sizeof(big));
    assert(buildTextMessageFrame(frame, sizeof(frame), BROADCAST_ADDR, 0, 1,
                                 big, sizeof(big)) == 0);

    printf("  buildTextMessageFrame: ok\n");
}

// ---------------------------------------------------------------------------
// FrameParser: deframing con risincronizzazione su spazzatura
// ---------------------------------------------------------------------------
static void testFrameParser() {
    uint8_t frame[64];
    size_t n = buildWantConfigFrame(frame, sizeof(frame), 0x1234);
    assert(n > 4);

    FrameParser p;
    uint8_t payload[MAX_PAYLOAD];

    // Spazzatura prima del frame (incluso uno 0x94 orfano)
    const uint8_t garbage[] = {0x00, 0x94, 0x11, 0xFF, 0x94, 0x94};
    for (uint8_t b : garbage) assert(p.feed(b, payload) == 0);
    // Nota: l'ultimo 0x94 lascia il parser in attesa di 0xC3: il frame vero
    // che inizia con 0x94 0xC3 viene comunque agganciato correttamente.
    size_t got = 0;
    for (size_t i = 0; i < n; ++i) {
        size_t r = p.feed(frame[i], payload);
        if (r) got = r;
    }
    assert(got == n - 4);
    assert(memcmp(payload, frame + 4, got) == 0);

    // Lunghezza oltre MAX_PAYLOAD -> scartata, il parser si risincronizza
    FrameParser p2;
    assert(p2.feed(FRAME_START1, payload) == 0);
    assert(p2.feed(FRAME_START2, payload) == 0);
    assert(p2.feed(0xFF, payload) == 0);
    assert(p2.feed(0xFF, payload) == 0);
    got = 0;
    for (size_t i = 0; i < n; ++i) {
        size_t r = p2.feed(frame[i], payload);
        if (r) got = r;
    }
    assert(got == n - 4);

    printf("  FrameParser: ok\n");
}

// ---------------------------------------------------------------------------
// parseFromRadio: eventi da payload FromRadio sintetici
// ---------------------------------------------------------------------------
struct TestHandler : FromRadioHandler {
    uint32_t myNode = 0;
    uint32_t nodeNum = 0;
    std::string shortName;
    uint32_t textFrom = 0;
    std::string text;
    uint32_t ackRequestId = 0;
    uint32_t ackError = 999;
    uint32_t configId = 0;

    void onMyNodeNum(uint32_t n) override { myNode = n; }
    void onNodeInfo(uint32_t n, const char* s, size_t sl, const char*, size_t) override {
        nodeNum = n;
        shortName.assign(s, sl);
    }
    void onTextMessage(uint32_t from, uint32_t, const char* t, size_t l) override {
        textFrom = from;
        text.assign(t, l);
    }
    void onRoutingResult(uint32_t req, uint32_t err) override {
        ackRequestId = req;
        ackError = err;
    }
    void onConfigComplete(uint32_t id) override { configId = id; }
};

static void testParseFromRadio() {
    uint8_t buf[256];
    TestHandler h;

    // FromRadio { 3: MyNodeInfo { 1: 0xDEADBEEF } }
    {
        uint8_t inner[16];
        ProtoWriter mi(inner, sizeof(inner));
        mi.varintField(1, 0xDEADBEEFu);
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(3, inner, mi.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.myNode == 0xDEADBEEFu);
    }

    // FromRadio { 4: NodeInfo { 1: num, 2: User { 3: "T9" } } }
    {
        uint8_t user[16];
        ProtoWriter u(user, sizeof(user));
        u.bytesField(3, reinterpret_cast<const uint8_t*>("T9"), 2);
        uint8_t node[32];
        ProtoWriter ni(node, sizeof(node));
        ni.varintField(1, 0x11223344u);
        ni.bytesField(2, user, u.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(4, node, ni.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.nodeNum == 0x11223344u);
        assert(h.shortName == "T9");
    }

    // FromRadio { 2: MeshPacket { 1: from, 4: Data { 1: TEXT, 2: "hola" } } }
    {
        uint8_t data[32];
        ProtoWriter d(data, sizeof(data));
        d.varintField(1, PORT_TEXT_MESSAGE);
        d.bytesField(2, reinterpret_cast<const uint8_t*>("hola"), 4);
        uint8_t pkt[64];
        ProtoWriter p(pkt, sizeof(pkt));
        p.varintField(1, 0x55u);
        p.bytesField(4, data, d.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(2, pkt, p.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.textFrom == 0x55u);
        assert(h.text == "hola");
    }

    // ACK: MeshPacket { 4: Data { 1: ROUTING, 2: Routing{}, 6: request_id } }
    // Routing vuoto = error_reason NONE = consegnato.
    {
        uint8_t data[32];
        ProtoWriter d(data, sizeof(data));
        d.varintField(1, PORT_ROUTING);
        d.bytesField(2, nullptr, 0);
        d.varintField(6, 0xABCDu);
        uint8_t pkt[64];
        ProtoWriter p(pkt, sizeof(pkt));
        p.bytesField(4, data, d.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(2, pkt, p.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.ackRequestId == 0xABCDu);
        assert(h.ackError == ROUTING_ERROR_NONE);
    }

    // NAK: Routing { 3: error_reason = 3 (timeout) }
    {
        uint8_t routing[8];
        ProtoWriter rt(routing, sizeof(routing));
        rt.varintField(3, 3);
        uint8_t data[32];
        ProtoWriter d(data, sizeof(data));
        d.varintField(1, PORT_ROUTING);
        d.bytesField(2, routing, rt.size());
        d.varintField(6, 0x99u);
        uint8_t pkt[64];
        ProtoWriter p(pkt, sizeof(pkt));
        p.bytesField(4, data, d.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(2, pkt, p.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.ackRequestId == 0x99u);
        assert(h.ackError == 3);
        assert(strcmp(routingErrorLabel(3), "timeout") == 0);
    }

    // FromRadio { 7: config_complete_id }
    {
        ProtoWriter fr(buf, sizeof(buf));
        fr.varintField(7, 0x42u);
        parseFromRadio(buf, fr.size(), h);
        assert(h.configId == 0x42u);
    }

    // Payload malformato: non deve andare in crash né emettere eventi falsi
    {
        TestHandler clean;
        const uint8_t junk[] = {0xFF, 0xFF, 0xFF, 0x02, 0x01};
        parseFromRadio(junk, sizeof(junk), clean);
        assert(clean.myNode == 0 && clean.text.empty());
    }

    printf("  parseFromRadio: ok\n");
}

int main() {
    testProtoRoundtrip();
    testBuildTextMessage();
    testFrameParser();
    testParseFromRadio();
    printf("test_meshproto: tutti i test superati.\n");
    return 0;
}
