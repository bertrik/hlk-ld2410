#include <string.h>             // memset


#include "ld2410-protocol.h"

LD2410Protocol::LD2410Protocol(void)
{
    _state = HEADER_F4;
    _sum = 0;
    memset(_buf, 0, sizeof(_buf));
    _len = 0;
    _idx = 0;
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
    _state = HEADER_F4;
    _len = 0;
}

bool LD2410Protocol::process_rx(uint8_t c)
{
    switch (_state) {
    case HEADER_F4:
        if (c == 0xF4) {
            _state = HEADER_F3;
        }
        break;
    case HEADER_F3:
        if (c == 0xF3) {
            _state = HEADER_F2;
        }
        break;
    case HEADER_F2:
        if (c == 0xF2) {
            _state = HEADER_F1;
        }
        break;
    case HEADER_F1:
        if (c == 0xF1) {
            _state = LEN_1;
        }
        break;
    case LEN_1:
        _len = c;
        _state = LEN_2;
        break;
    case LEN_2:
        _len += (c >> 8);
        if (_len < sizeof(_buf)) {
            _idx = 0;
            _state = (_len > 0) ? DATA : FOOTER_F8;
        } else {
            _state = HEADER_F4;
        }
        break;
    case DATA:
        _buf[_idx++] = c;
        if (_idx == _len) {
            _state = FOOTER_F8;
        }
        break;
    case FOOTER_F8:
        _state = (c == 0xF8) ? FOOTER_F7 : HEADER_F4;
        break;
    case FOOTER_F7:
        _state = (c == 0xF7) ? FOOTER_F6 : HEADER_F4;
        break;
    case FOOTER_F6:
        _state = (c == 0xF6) ? FOOTER_F5 : HEADER_F4;
        break;
    case FOOTER_F5:
        _state = HEADER_F4;
        return c == 0xF5;
    default:
        _state = HEADER_F4;
        break;
    }
    return false;
}

size_t LD2410Protocol::get_data(uint8_t * data)
{
    memcpy(data, _buf, _len);
    return _len;
}
