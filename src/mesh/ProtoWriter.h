#pragma once

#include <stddef.h>
#include <stdint.h>

// Encoder protobuf minimale (solo varint e campi length-delimited: è tutto
// ciò che serve per i messaggi ToRadio del client API di Meshtastic).
// Nessuna dipendenza da Arduino: testabile in nativo.
class ProtoWriter {
public:
    ProtoWriter(uint8_t* buf, size_t cap) : _buf(buf), _cap(cap) {}

    bool varintField(uint32_t field, uint64_t value);
    bool bytesField(uint32_t field, const uint8_t* data, size_t len);

    size_t size() const { return _pos; }
    bool ok() const { return !_overflow; }

private:
    bool putByte(uint8_t b);
    bool putVarint(uint64_t v);
    bool putKey(uint32_t field, uint8_t wireType);

    uint8_t* _buf;
    size_t _cap;
    size_t _pos = 0;
    bool _overflow = false;
};
