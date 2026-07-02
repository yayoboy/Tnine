#include "MeshtasticLink.h"
#include "config.h"

// Serial2 = UART1 sul core arduino-pico (earlephilhower).

void MeshtasticLink::begin(RxCallback onRx) {
    _onRx = onRx;
    Serial2.setTX(PIN_MESH_TX);
    Serial2.setRX(PIN_MESH_RX);
    Serial2.begin(MESH_BAUD);
    _rxLine.reserve(MESSAGE_MAX_LEN + 16);
}

void MeshtasticLink::poll() {
    while (Serial2.available() > 0) {
        char c = static_cast<char>(Serial2.read());
        if (c == '\n' || c == '\r') {
            if (_rxLine.length() > 0) {
                if (_onRx) _onRx(_rxLine);
                _rxLine = "";
            }
        } else if (_rxLine.length() < MESSAGE_MAX_LEN + 15) {
            _rxLine += c;
        }
    }
}

bool MeshtasticLink::sendText(const String& msg) {
    if (msg.length() == 0) return false;
    Serial2.print(msg);
    Serial2.print('\n');
    return true;
}
