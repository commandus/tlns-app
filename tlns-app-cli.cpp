#include <cstring>
#include <cstdlib>
#include <iostream>

#include "linenoise.h"

#include "lorawan/file-helper.h"
#include "lorawan/proto/payload2device/payload2device-parser.h"
#include "lorawan/lorawan-string.h"
#include "lorawan/lorawan-date.h"

static const char *CMDS[] {
    "ping",     // 0
    "send",     // 1
    "quit"      // 2
};

static const char *SEND_OPTIONS[] {
    "payload",  // 0
    "fopts",    // 1
    "at",       // 2
    "fport",    // 3
    "proto"     // 4
};

static void completionHook (
    char const* prefix,
    linenoiseCompletions* lc
)
{
    if (prefix[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
    return;
    std::string expr(prefix);
    trim(expr);
    if (expr.empty()) {
        for (auto &c : CMDS) {
            linenoiseAddCompletion(lc, c);
        }
        return;
    }
    Payload2DeviceParser p;
    p.parse(expr);
    if (p.command == PAYLOAD2DEVICE_COMMAND_SEND) {
        if (p.payload.empty()) {
            linenoiseAddCompletion(lc, SEND_OPTIONS[0]);    // payload
        }
        if (p.fopts.empty()) {
            linenoiseAddCompletion(lc, SEND_OPTIONS[1]);    // fopts
        }
        if (p.tim == 0) {
            linenoiseAddCompletion(lc, SEND_OPTIONS[2]);    // at
        }
        if (p.fport == 0) {
            linenoiseAddCompletion(lc, SEND_OPTIONS[3]);    // fport
        }
        if (p.proto == 0) {
            linenoiseAddCompletion(lc, SEND_OPTIONS[4]);    // proto
        }

        for (auto &o : SEND_OPTIONS) {
            linenoiseAddCompletion(lc, o);
        }
        return;
    }
}

static std::string now;

static char *hintsHook(
    const char *expr,
    int *color,
    int *bold
) {
    Payload2DeviceParser p;
    p.parse(expr);
    if (p.command == PAYLOAD2DEVICE_COMMAND_SEND) {
        if (!p.hasSendOptionValue(p.lastSendOption)) {
            switch (p.lastSendOption) {
                case PAYLOAD2DEVICE_PARSER_STATE_ADDRESS:
                    *color = 32;
                    *bold = 0;
                    return " <address>";
                case PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD:
                    *color = 32;
                    *bold = 0;
                    return " <hex-string>";
                case PAYLOAD2DEVICE_PARSER_STATE_FOPTS:
                    *color = 32;
                    *bold = 0;
                    return " <hex-string>";
                case PAYLOAD2DEVICE_PARSER_STATE_FPORT:
                    *color = 32;
                    *bold = 0;
                    return " <0..255>";
                case PAYLOAD2DEVICE_PARSER_STATE_TIME:
                    *color = 32;
                    *bold = 0;
                    now = " <" + time2string(time(nullptr)) + ">";
                    return (char *) now.c_str();
                case PAYLOAD2DEVICE_PARSER_STATE_PROTO:
                    *color = 32;
                    *bold = 0;
                    return " <0..255>";
                default:
                    break;
            }
        } else {
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_ADDRESS)) {
                *color = 35;
                *bold = 1;
                return " <address>";
            }
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD)) {
                *color = 35;
                *bold = 1;
                return " payload";
            }
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_FOPTS)) {
                *color = 35;
                *bold = 1;
                return " fopts";
            }
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_FPORT)) {
                *color = 35;
                *bold = 1;
                return " fport";
            }
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_TIME)) {
                *color = 35;
                *bold = 1;
                return " at";
            }
            if (!p.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_PROTO)) {
                *color = 35;
                *bold = 1;
                return " proto";
            }
        }
    }
    return nullptr;
}

static PAYLOAD2DEVICE_COMMAND processInput(
    const std::string &expr
)
{
    Payload2DeviceParser p;
    p.parse(expr);
    std::cout << p.toString() << "\n";
    return p.command;
}

int main (
    int argc,
    char** argv
) {
    const std::string historyFileName (getHomeDir() + "/tlns-app-cli.history");
    //linenoiseSetCompletionCallback(completionHook);
    linenoiseSetHintsCallback(hintsHook);
    linenoiseHistoryLoad(historyFileName.c_str());

    char const* prompt = "tlns> ";

    while (true) {
        char* l = linenoise(prompt);
        std::string line(l);
        free(l);
        if (processInput(line) == PAYLOAD2DEVICE_COMMAND_QUIT)
            break;
        linenoiseHistoryAdd(line.c_str());
    }
    linenoiseHistorySave(historyFileName.c_str());
}
