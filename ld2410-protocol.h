#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// parsing state
typedef enum {
    HEADER,
    LEN_1,
    LEN_2,
    DATA,
    FOOTER,
} state_t;

typedef enum {
    LD303_CMD_ENABLE_CONFIG = 0x00FF,   // 0001
    LD303_CMD_END_CONFIG = 0x00FE,      // none

    LD303_CMD_MAX_DISTANCE_GATE = 0x0060,       // max moving distance, max resting distance, no-one duration
    LD303_CMD_READ_PARAMETER = 0x0061,  // none
    LD303_CMD_ENABLE_ENGINEERING = 0x0062,      // none
    LD303_CMD_CLOSE_ENGINEERING = 0x0063,       // none
    LD303_CMD_SET_SENSITIVITY = 0x0064, // distance gate, motion sensitivity, static sensitivity

    LD303_CMD_READ_FIRMWARE_VERSION = 0x00A0,   // none
    LD303_CMD_SET_BAUD_RATE = 0x00A1,   // baud selection index
    LD303_CMD_FACTORY_RESET = 0x00A2,   // none
    LD303_CMD_RESTART = 0x00A3, // none

} ld303_cmd_t;

class LD2410Protocol {

  private:
    state_t _state;
    uint8_t _buf[64];
    uint8_t _len;
    uint8_t _idx;
    uint32_t _delim;
    uint32_t _header;
    uint32_t _footer;

  public:
    // FAFBFCFD/01020304 for ACK
    // F1F2F3F4/F5F6F7F8 for report
    LD2410Protocol(uint32_t header, uint32_t footer);

    // builds a command
    size_t build_command(uint8_t * buf, uint16_t cmd, uint16_t cmd_data_len,
                         const uint8_t * cmd_data);

    // processes received data, returns true if measurement data was found
    bool process_rx(uint8_t c);
    void reset_rx(void);

    // call this when process_rx returns true, copies received data into buffer, returns length
    size_t get_data(uint8_t * data);

};
