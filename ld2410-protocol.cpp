#include <string.h> // memset


#include "ld2410-protocol.h"

LD2410Protocol::LD2410Protocol(void)
{
    _state = STATE_HEADER_55;
    _sum = 0;
    memset(_buf, 0, sizeof(_buf));
    _len = 0;
    _idx = 0;
}

size_t LD2410Protocol::build_command(uint8_t *buf, uint16_t cmd, uint16_t cmd_data_len, const uint8_t *cmd_data)
{
    // header
    size_t idx = 0;
    buf[idx++] = 0xFD;
    buf[idx++] = 0xFC;
    buf[idx++] = 0xFB;
    buf[idx++] = 0xFA;
    // length
    uint16_t len = 2 + cmd_data_len;
    buf[idx++] = (len >> 8) & 0xFF;
    buf[idx++] = len & 0xFF;
    // command word
    buf[idx++] = (cmd >> 8) & 0xFF;
    buf[idx++] = cmd & 0xFF;
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

size_t LD2410Protocol::build_query(uint8_t *buf, const uint8_t *data, size_t len)
{
    return 0;
}

bool LD2410Protocol::process_rx(uint8_t c)
{
    return 0;
}

size_t LD2410Protocol::get_data(uint8_t *data)
{
    return 0;
}
    
    
