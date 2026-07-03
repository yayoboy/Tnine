#include "ProtoWriter.h"

bool ProtoWriter::putByte(uint8_t b) {
    if (_pos >= _cap) {
        _overflow = true;
        return false;
    }
    _buf[_pos++] = b;
    return true;
}

bool ProtoWriter::putVarint(uint64_t v) {
    do {
        uint8_t b = v & 0x7F;
        v >>= 7;
        if (v) b |= 0x80;
        if (!putByte(b)) return false;
    } while (v);
    return true;
}

bool ProtoWriter::putKey(uint32_t field, uint8_t wireType) {
    return putVarint((static_cast<uint64_t>(field) << 3) | wireType);
}

bool ProtoWriter::varintField(uint32_t field, uint64_t value) {
    return putKey(field, 0) && putVarint(value);
}

bool ProtoWriter::bytesField(uint32_t field, const uint8_t* data, size_t len) {
    if (!putKey(field, 2) || !putVarint(len)) return false;
    for (size_t i = 0; i < len; ++i) {
        if (!putByte(data[i])) return false;
    }
    return true;
}
