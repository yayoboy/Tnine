#pragma once

#include <stddef.h>
#include <stdint.h>

// Decoder protobuf minimale: itera i campi di un messaggio gestendo i wire
// type 0 (varint), 2 (length-delimited), 5 (fixed32) e 1 (fixed64).
// I campi non riconosciuti vengono semplicemente saltati dal chiamante.
class ProtoReader {
public:
    ProtoReader(const uint8_t* buf, size_t len) : _buf(buf), _len(len) {}

    // Avanza al campo successivo; false a fine messaggio o su dati malformati.
    bool next();

    uint32_t field() const { return _field; }
    uint8_t wireType() const { return _wireType; }

    // Valido con wireType 0.
    uint64_t varint() const { return _varint; }
    // Validi con wireType 2.
    const uint8_t* data() const { return _data; }
    size_t dataLen() const { return _dataLen; }

private:
    bool readVarint(uint64_t& out);

    const uint8_t* _buf;
    size_t _len;
    size_t _pos = 0;

    uint32_t _field = 0;
    uint8_t _wireType = 0;
    uint64_t _varint = 0;
    const uint8_t* _data = nullptr;
    size_t _dataLen = 0;
};
