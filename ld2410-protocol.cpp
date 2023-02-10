#include <string.h>             // memset


#include "ld2410-protocol.h"

LD2410Protocol::LD2410Protocol(uint32_t header, uint32_t footer)
{
    _header = header;
    _footer = footer;
    reset_rx();
}

size_t LD2410Protocol::build_command(uint8_t * buf, uint16_t cmd, uint16_t cmd_data_len,
                                     const uint8_t * cmd_data)
{
    // header
    size_t idx = 0;
    buf[idx++] = 0xFD;
    buf[idx++] = 0xFC;
    buf[idx++] = 0xFB;
    buf[idx++] = 0xFA;
    // length
    uint16_t len = 2 + cmd_data_len;
    buf[idx++] = len & 0xFF;
    buf[idx++] = (len >> 8) & 0xFF;
    // command word
    buf[idx++] = cmd & 0xFF;
    buf[idx++] = (cmd >> 8) & 0xFF;
    // command data
    for (int i = 0; i < cmd_data_len; i++) {
        buf[idx++] = cmd_data[i];
    }
    // footer
    buf[idx++] = 0x04;
    buf[idx++] = 0x03;
    buf[idx++] = 0x02;
    buf[idx++] = 0x01;
    return idx;
}

void LD2410Protocol::reset_rx(void)
{
    _delim = _header;
    _state = HEADER;
    memset(_buf, 0, sizeof(_buf));
    _len = 0;
    _idx = 0;
}

bool LD2410Protocol::process_rx(uint8_t c)
{
    switch (_state) {
    case HEADER:
        if (c == (_delim & 0xFF)) {
            _delim = _delim >> 8;
            if (_delim == 0) {
                _state = LEN_1;
            }
        } else {
            reset_rx();
        }
        break;
    case LEN_1:
        _len = c;
        _state = LEN_2;
        break;
    case LEN_2:
        _len += (c >> 8);
        _delim = _footer;
        if (_len < sizeof(_buf)) {
            _state = (_len > 0) ? DATA : FOOTER;
        } else {
            _state = HEADER;
        }
        break;
    case DATA:
        _buf[_idx++] = c;
        if (_idx == _len) {
            _state = FOOTER;
        }
        break;
    case FOOTER:
        if (c == (_delim & 0xFF)) {
            _delim = _delim >> 8;
            if (_delim == 0) {
                return true;
            }
        } else {
            reset_rx();
        }
        break;
    default:
        reset_rx();
        break;
    }
    return false;
}

size_t LD2410Protocol::get_data(uint8_t * data)
{
    memcpy(data, _buf, _len);
    return _len;
}
