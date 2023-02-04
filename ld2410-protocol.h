#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// parsing state
typedef enum {
    STATE_HEADER_55,
    STATE_HEADER_A5,
    STATE_LEN,
    STATE_DATA,
    STATE_CHECK
} state_t;

class LD2410Protocol {

private:
    state_t _state;
    uint8_t _sum;
    uint8_t _buf[32];
    uint8_t _len;
    uint8_t _idx;

public:
    LD2410Protocol();

    // builds a command
    size_t build_command(uint8_t *buf, uint16_t cmd, uint16_t cmd_data_len, const uint8_t *cmd_data);

    // queries the module for measurement data (param usually 0xD3)
    size_t build_query(uint8_t *buf, const uint8_t *data, size_t len);

    // processes received data, returns true if measurement data was found
    bool process_rx(uint8_t c);

    // call this when process_rx returns true, copies received data into buffer, returns length
    size_t get_data(uint8_t *data);

};

