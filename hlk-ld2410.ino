#include <Arduino.h>
#include <SoftwareSerial.h>

#include "ld2410-protocol.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "editline.h"
#include "cmdproc.h"

#define RADAR_BAUD_RATE 256000
#define PIN_TX  D3
#define PIN_RX  D2
#define printf Serial.printf

static LD2410Protocol proto_ack = LD2410Protocol(0xFAFBFCFD, 0x01020304);
static LD2410Protocol proto_rsp = LD2410Protocol(0xF1F2F3F4, 0xF5F6F7F8);
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

static int do_reboot(int argc, char *argv[])
{
    ESP.restart();
    return CMD_OK;
}

static int do_debug(int argc, char *argv[])
{
    debug = !debug;
    return CMD_OK;
}

static int do_config_mode(int argc, char *argv[])
{
    uint8_t buf[32];
    uint8_t data[] = { 0x01, 0x00 };
    size_t len = proto_rsp.build_command(buf, LD303_CMD_ENABLE_CONFIG, sizeof(data), data);
    printhex(">", buf, len);
    radar.write(buf, len);
    return CMD_OK;
}

static int do_report_mode(int argc, char *argv[])
{
    uint8_t buf[32];
    size_t len = proto_rsp.build_command(buf, LD303_CMD_END_CONFIG, 0, NULL);
    printhex(">", buf, len);
    radar.write(buf, len);
    return CMD_OK;
}

static int do_baud(int argc, char *argv[])
{
    uint8_t baud_index = 1;

    if (argc > 1) {
        baud_index = atoi(argv[1]);
    }
    printf("Setting baud index %d\n", baud_index);

    uint8_t buf[32];
    size_t len;

    // open config
    uint8_t cmd_open[2] = { 1, 0 };
    len = proto_rsp.build_command(buf, LD303_CMD_ENABLE_CONFIG, 2, cmd_open);
    radar.write(buf, len);

    // baud
    uint8_t cmd_baud[2] = { baud_index, 0 };
    len = proto_rsp.build_command(buf, LD303_CMD_SET_BAUD_RATE, 2, cmd_baud);
    radar.write(buf, len);

    // close config
    len = proto_rsp.build_command(buf, LD303_CMD_END_CONFIG, 0, NULL);
    radar.write(buf, len);

    return CMD_OK;
}

// tries to detect the baud rate by attempting baudrates until we get a reply
static int do_autobaud(int argc, char *argv[])
{
    static const int baudrates[] = { 256000, 9600, 19200, 38400, 57600, 115200, 230400, 460800 };

    bool found = false;
    for (size_t i = 0; i < sizeof(baudrates) / sizeof(*baudrates); i++) {
        int baudrate = baudrates[i];
        printf("Attempting baud rate %d ", baudrate);
        radar.begin(baudrate);
        proto_rsp.reset_rx();
        unsigned int start = millis();
        while (!found && ((millis() - start) < 3000)) {
            if (radar.available()) {
                printf(".");
                uint8_t c = radar.read();
                found = proto_rsp.process_rx(c);
            }
        }
        printf("\n");
        if (found) {
            printf("Succeeded at baud rate %d!\n", baudrate);
            return CMD_OK;
        }
    }
    printf("Failed to determine baud rate ... :(\n");
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
    { "reboot", do_reboot, "Reboot" },
    { "debug", do_debug, "Toggle debug mode" },
    { "c", do_config_mode, "Enter config mode" },
    { "r", do_report_mode, "Enter report mode" },
    { "baud", do_baud, "<baudrate> Set baud rate" },
    { "auto", do_autobaud, "Determine baudrate automatically" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    return show_help(commands);
}

void setup(void)
{
    Serial.begin(115200);
    pinMode(PIN_TX, OUTPUT);
    radar.begin(256000);

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
        if (proto_ack.process_rx(c)) {
            int len = proto_ack.get_data(buf);
            printhex("ACK <", buf, len);
        }
        if (proto_rsp.process_rx(c)) {
            int len = proto_rsp.get_data(buf);
            printhex("RSP <", buf, len);
        }
    }
}
