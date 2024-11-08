#include <cstring>
#include <cstdlib>
#include <iostream>

#include "linenoise.h"

#include "lorawan/file-helper.h"
#include "lorawan/proto/payload2device/payload2device-parser.h"
#include "lorawan/lorawan-string.h"
#include "lorawan/lorawan-date.h"

static std::string now;
static std::string lastExpression;
static Payload2DeviceParser parser;

static char* HINTS[] {
    "",                     // PAYLOAD2DEVICE_PARSER_STATE_START
    " <address> payload",   // PAYLOAD2DEVICE_PARSER_STATE_COMMAND
    " <address> payload",   // PAYLOAD2DEVICE_PARSER_STATE_ADDRESS
    " 0..255",              // PAYLOAD2DEVICE_PARSER_STATE_FPORT
    " <hex-string>",        // PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD
    " <hex-string>",        // PAYLOAD2DEVICE_PARSER_STATE_FOPTS
    "YYYY-MM-DDThh:mm:ss+ZONE",// PAYLOAD2DEVICE_PARSER_STATE_TIME
    " 0..255"               // PAYLOAD2DEVICE_PARSER_STATE_PROTO
};

static char *hintsHook(
    const char *expr,
    int *color,
    int *bold
) {
    lastExpression = expr;
    parser.parse(expr, lastExpression.length());
    if (parser.command == PAYLOAD2DEVICE_COMMAND_SEND) {
        if (!parser.hasSendOptionValue(parser.lastSendOption)) {
            switch (parser.lastSendOption) {
                case PAYLOAD2DEVICE_PARSER_STATE_TIME:
                    *color = 93;
                    *bold = 0;
                    now = " <" + time2string(time(nullptr)) + ">";
                    return (char *) now.c_str();
                default:
                    *color = 93;
                    *bold = 0;
                    return HINTS[parser.lastSendOption];
            }
        } else {
            if (parser.lastSendOption == PAYLOAD2DEVICE_PARSER_STATE_ADDRESS) {
                *color = 32;
                *bold = 1;
                return " payload";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_ADDRESS)) {
                *color = 32;
                *bold = 1;
                return " <address>";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD)) {
                *color = 32;
                *bold = 0;
                return " payload";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_FOPTS)) {
                *color = 32;
                *bold = 1;
                return " fopts";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_FPORT)) {
                *color = 32;
                *bold = 1;
                return " fport";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_TIME)) {
                *color = 32;
                *bold = 1;
                return " at";
            }
            if (!parser.hasSendOptionName(PAYLOAD2DEVICE_PARSER_STATE_PROTO)) {
                *color = 32;
                *bold = 1;
                return " proto";
            }
        }
    }
    return nullptr;
}

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
) {
    auto len = strlen(prefix);
    if (len == 0 && parser.state <= PAYLOAD2DEVICE_PARSER_STATE_COMMAND) {
        for (auto &c: CMDS) {
            linenoiseAddCompletion(lc, c);
        }
    }

    std::string s = Payload2DeviceParser::completion(lastExpression);
    linenoiseAddCompletion(lc, s.c_str());
}

static PAYLOAD2DEVICE_COMMAND processInput(
    const char *expr
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
    linenoiseSetCompletionCallback(completionHook);
    linenoiseSetHintsCallback(hintsHook);
    linenoiseHistoryLoad(historyFileName.c_str());

    char const* prompt = "tlns> ";

    while (true) {
        char* l = linenoise(prompt);

        if (l) {
            auto cmd = processInput(l);
            linenoiseHistoryAdd(l);
            free(l);
            if (cmd == PAYLOAD2DEVICE_COMMAND_QUIT)
                break;
        }
    }
    linenoiseHistorySave(historyFileName.c_str());
}
