#include "ProtoReader.h"

bool ProtoReader::readVarint(uint64_t& out) {
    out = 0;
    for (uint8_t shift = 0; shift < 64; shift += 7) {
        if (_pos >= _len) return false;
        uint8_t b = _buf[_pos++];
        out |= static_cast<uint64_t>(b & 0x7F) << shift;
        if (!(b & 0x80)) return true;
    }
    return false;  // varint troppo lungo
}

bool ProtoReader::next() {
    if (_pos >= _len) return false;

    uint64_t key;
    if (!readVarint(key)) return false;
    _field = static_cast<uint32_t>(key >> 3);
    _wireType = key & 0x07;

    switch (_wireType) {
        case 0:  // varint
            return readVarint(_varint);
        case 2: {  // length-delimited
            uint64_t len;
            if (!readVarint(len)) return false;
            if (len > _len - _pos) return false;
            _data = _buf + _pos;
            _dataLen = static_cast<size_t>(len);
            _pos += _dataLen;
            return true;
        }
        case 5:  // fixed32 (little-endian)
            if (_len - _pos < 4) return false;
            _fixed32 = static_cast<uint32_t>(_buf[_pos]) |
                       (static_cast<uint32_t>(_buf[_pos + 1]) << 8) |
                       (static_cast<uint32_t>(_buf[_pos + 2]) << 16) |
                       (static_cast<uint32_t>(_buf[_pos + 3]) << 24);
            _pos += 4;
            return true;
        case 1:  // fixed64
            if (_len - _pos < 8) return false;
            _pos += 8;
            return true;
        default:
            return false;  // wire type non supportato
    }
}
