#include <Arduino.h>
#include <SoftwareSerial.h>

#include "ld2410-protocol.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "editline.h"
#include "cmdproc.h"

#define PIN_TX  D2
#define PIN_RX  D3
#define printf Serial.printf

static LD2410Protocol protocol;
static SoftwareSerial radar(PIN_RX, PIN_TX);
static char cmdline[128];
static bool debug = false;

static void printhex(const char *prefix, const uint8_t * buf, size_t len)
{
    printf(prefix);
    for (size_t i = 0; i < len; i++) {
        printf(" %02X", buf[i]);
    }
    printf("\n");
}

static int do_baud(int argc, char *argv[])
{
    uint8_t buf[32];

    if (argc < 2) {
        return CMD_ARG;
    }
    int baud = atoi(argv[1]);
    printf("Setting baud %d\n", baud);
//    size_t len = protocol.build_command(buf, CMD_BAUD_RATE, baud / 100);
//    radar_write(buf, len);

    delay(100);
    radar.begin(baud);

    return CMD_OK;
}

static int show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        printf("%10s: %s\n", cmd->name, cmd->help);
    }
    return CMD_OK;
}

static int do_help(int argc, char *argv[]);

static const cmd_t commands[] = {
    { "help", do_help, "Show help" },
    { "baud", do_baud, "<baudrate> Set baud rate" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    return show_help(commands);
}

void setup(void)
{
    Serial.begin(115200);
    radar.begin(115200);

    EditInit(cmdline, sizeof(cmdline));
}

void loop(void)
{
    uint8_t buf[256];

    // parse command line
    if (Serial.available()) {
        char c;
        bool haveLine = EditLine(Serial.read(), &c);
        Serial.write(c);
        if (haveLine) {
            int result = cmd_process(commands, cmdline);
            switch (result) {
            case CMD_OK:
                printf("OK\n");
                break;
            case CMD_NO_CMD:
                break;
            case CMD_ARG:
                printf("Invalid arguments\n");
                break;
            case CMD_UNKNOWN:
                printf("Unknown command, available commands:\n");
                show_help(commands);
                break;
            default:
                printf("%d\n", result);
                break;
            }
            printf(">");
        }
    }
    // process incoming data from radar
    while (radar.available()) {
        uint8_t c = radar.read();
        if (debug) {
            printf(" %02X", c);
        }
        // run receive state machine
        bool done = protocol.process_rx(c);
        if (done) {
            int len = protocol.get_data(buf);
            printhex("<", buf, len);
            uint8_t cmd = buf[0];
        }
    }
}
