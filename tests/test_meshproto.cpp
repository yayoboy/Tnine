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

// Aggiunge a mano un campo fixed32 (little-endian) a un buffer protobuf:
// ProtoWriter scrive solo varint/bytes, ma FromRadio usa fixed32 per
// snr/last_heard/coordinate.
static size_t appendFixed32(uint8_t* buf, size_t pos, uint32_t field, uint32_t value) {
    buf[pos++] = static_cast<uint8_t>((field << 3) | 5);
    buf[pos++] = value & 0xFF;
    buf[pos++] = (value >> 8) & 0xFF;
    buf[pos++] = (value >> 16) & 0xFF;
    buf[pos++] = (value >> 24) & 0xFF;
    return pos;
}

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
    size_t pos = w.size();
    pos = appendFixed32(buf, pos, 4, 0x12345678u);

    ProtoReader r(buf, pos);
    assert(r.next() && r.field() == 1 && r.varint() == 42);
    assert(r.next() && r.field() == 2 && r.varint() == 0xFFFFFFFFu);
    assert(r.next() && r.field() == 3 && r.dataLen() == 4);
    assert(memcmp(r.data(), "ciao", 4) == 0);
    assert(r.next() && r.field() == 4 && r.wireType() == 5);
    assert(r.fixed32() == 0x12345678u);
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
                                     0x11223344u, 2, 0xABCD, "ciao à", 7);
    assert(n > 4);
    assert(frame[0] == FRAME_START1 && frame[1] == FRAME_START2);
    size_t payloadLen = (static_cast<size_t>(frame[2]) << 8) | frame[3];
    assert(payloadLen == n - 4);

    // ToRadio { 1: MeshPacket }
    ProtoReader toRadio(frame + 4, payloadLen);
    assert(toRadio.next() && toRadio.field() == 1 && toRadio.wireType() == 2);

    // MeshPacket { to, channel, decoded, id, want_ack }
    uint32_t to = 0, channel = 0, id = 0, wantAck = 0;
    const uint8_t* decoded = nullptr;
    size_t decodedLen = 0;
    ProtoReader pkt(toRadio.data(), toRadio.dataLen());
    while (pkt.next()) {
        switch (pkt.field()) {
            case 2:  to = pkt.varint(); break;
            case 3:  channel = pkt.varint(); break;
            case 4:  decoded = pkt.data(); decodedLen = pkt.dataLen(); break;
            case 6:  id = pkt.varint(); break;
            case 10: wantAck = pkt.varint(); break;
        }
    }
    assert(to == 0x11223344u);
    assert(channel == 2);
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
    NodeInfoData nodeInfo;
    std::string shortName, longName;
    uint32_t chIndex = 99, chRole = 99;
    std::string chName;
    uint32_t textFrom = 0;
    std::string text;
    uint32_t posFrom = 0;
    int32_t lat = 0, lon = 0;
    uint32_t telemFrom = 0, battery = 0;
    uint32_t ackRequestId = 0;
    uint32_t ackError = 999;
    uint32_t configId = 0;
    bool rebooted = false;

    void onMyNodeNum(uint32_t n) override { myNode = n; }
    void onNodeInfo(const NodeInfoData& info) override {
        nodeInfo = info;
        shortName.assign(info.shortName ? info.shortName : "", info.shortLen);
        longName.assign(info.longName ? info.longName : "", info.longLen);
    }
    void onChannel(uint32_t idx, const char* name, size_t nameLen, uint32_t role) override {
        chIndex = idx;
        chRole = role;
        chName.assign(name ? name : "", nameLen);
    }
    void onTextMessage(uint32_t from, uint32_t, const char* t, size_t l) override {
        textFrom = from;
        text.assign(t, l);
    }
    void onPosition(uint32_t from, int32_t la, int32_t lo) override {
        posFrom = from; lat = la; lon = lo;
    }
    void onTelemetry(uint32_t from, uint32_t batt) override {
        telemFrom = from; battery = batt;
    }
    void onRoutingResult(uint32_t req, uint32_t err) override {
        ackRequestId = req;
        ackError = err;
    }
    void onConfigComplete(uint32_t id) override { configId = id; }
    void onRebooted() override { rebooted = true; }
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

    // FromRadio { 4: NodeInfo { num, User{long,short}, snr(f32),
    //                           last_heard(f32), DeviceMetrics{battery} } }
    {
        uint8_t user[48];
        ProtoWriter u(user, sizeof(user));
        u.bytesField(2, reinterpret_cast<const uint8_t*>("Tastiera T9"), 11);
        u.bytesField(3, reinterpret_cast<const uint8_t*>("T9"), 2);
        uint8_t metrics[8];
        ProtoWriter m(metrics, sizeof(metrics));
        m.varintField(1, 87);  // battery_level
        uint8_t node[128];
        ProtoWriter ni(node, sizeof(node));
        ni.varintField(1, 0x11223344u);
        ni.bytesField(2, user, u.size());
        size_t pos = ni.size();
        float snr = 7.5f;
        uint32_t snrBits;
        memcpy(&snrBits, &snr, 4);
        pos = appendFixed32(node, pos, 4, snrBits);        // snr
        pos = appendFixed32(node, pos, 5, 1700000000u);    // last_heard
        ProtoWriter ni2(node + pos, sizeof(node) - pos);
        ni2.bytesField(6, metrics, m.size());
        pos += ni2.size();
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(4, node, pos);
        parseFromRadio(buf, fr.size(), h);
        assert(h.nodeInfo.num == 0x11223344u);
        assert(h.shortName == "T9");
        assert(h.longName == "Tastiera T9");
        assert(h.nodeInfo.snr == 7.5f);
        assert(h.nodeInfo.lastHeard == 1700000000u);
        assert(h.nodeInfo.batteryLevel == 87);
    }

    // FromRadio { 10: Channel { 1: index, 2: Settings{ 3: name }, 3: role } }
    {
        uint8_t settings[24];
        ProtoWriter s(settings, sizeof(settings));
        s.bytesField(3, reinterpret_cast<const uint8_t*>("Escursione"), 10);
        uint8_t ch[48];
        ProtoWriter c(ch, sizeof(ch));
        c.varintField(1, 2);
        c.bytesField(2, settings, s.size());
        c.varintField(3, CHANNEL_ROLE_SECONDARY);
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(10, ch, c.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.chIndex == 2);
        assert(h.chRole == CHANNEL_ROLE_SECONDARY);
        assert(h.chName == "Escursione");
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

    // Posizione: Data { 1: POSITION, 2: Position{ lat/lon sfixed32 } }
    {
        uint8_t posBuf[16];
        size_t pos = 0;
        pos = appendFixed32(posBuf, pos, 1, static_cast<uint32_t>(450689000));   // 45.0689000
        pos = appendFixed32(posBuf, pos, 2, static_cast<uint32_t>(-76825000));   // -7.6825000
        uint8_t data[48];
        ProtoWriter d(data, sizeof(data));
        d.varintField(1, PORT_POSITION);
        d.bytesField(2, posBuf, pos);
        uint8_t pkt[64];
        ProtoWriter p(pkt, sizeof(pkt));
        p.varintField(1, 0x66u);
        p.bytesField(4, data, d.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(2, pkt, p.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.posFrom == 0x66u);
        assert(h.lat == 450689000);
        assert(h.lon == -76825000);
    }

    // Telemetria: Data { 1: TELEMETRY, 2: Telemetry{ 2: DeviceMetrics{1: 42} } }
    {
        uint8_t metrics[8];
        ProtoWriter m(metrics, sizeof(metrics));
        m.varintField(1, 42);
        uint8_t telem[16];
        ProtoWriter t(telem, sizeof(telem));
        t.bytesField(2, metrics, m.size());
        uint8_t data[32];
        ProtoWriter d(data, sizeof(data));
        d.varintField(1, PORT_TELEMETRY);
        d.bytesField(2, telem, t.size());
        uint8_t pkt[64];
        ProtoWriter p(pkt, sizeof(pkt));
        p.varintField(1, 0x77u);
        p.bytesField(4, data, d.size());
        ProtoWriter fr(buf, sizeof(buf));
        fr.bytesField(2, pkt, p.size());
        parseFromRadio(buf, fr.size(), h);
        assert(h.telemFrom == 0x77u);
        assert(h.battery == 42);
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

    // FromRadio { 7: config_complete_id } e { 8: rebooted }
    {
        ProtoWriter fr(buf, sizeof(buf));
        fr.varintField(7, 0x42u);
        fr.varintField(8, 1);
        parseFromRadio(buf, fr.size(), h);
        assert(h.configId == 0x42u);
        assert(h.rebooted);
    }

    // Payload malformato: non deve andare in crash né emettere eventi falsi
    {
        TestHandler clean;
        const uint8_t junk[] = {0xFF, 0xFF, 0xFF, 0x02, 0x01};
        parseFromRadio(junk, sizeof(junk), clean);
        assert(clean.myNode == 0 && clean.text.empty() && !clean.rebooted);
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
