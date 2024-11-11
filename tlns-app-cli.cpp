#include <cstring>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "linenoise.h"

#include "lorawan/file-helper.h"
#include "lorawan/proto/payload2device/payload2device-parser.h"
#include "lorawan/lorawan-string.h"
#include "lorawan/lorawan-date.h"
#include "lorawan/lorawan-error.h"


static std::string now;
static std::string lastExpression;
static Payload2DeviceParser parser;

static const char* HINTS_VALUE[] {
    "",                     // PAYLOAD2DEVICE_PARSER_STATE_START
    " <address> payload",   // PAYLOAD2DEVICE_PARSER_STATE_COMMAND
    " <address> payload",   // PAYLOAD2DEVICE_PARSER_STATE_ADDRESS
    " 0..255",              // PAYLOAD2DEVICE_PARSER_STATE_FPORT
    " <hex-string>",        // PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD
    " <hex-string>",        // PAYLOAD2DEVICE_PARSER_STATE_FOPTS
    "YYYY-MM-DDThh:mm:ss+ZONE",// PAYLOAD2DEVICE_PARSER_STATE_TIME
    " 0..255"               // PAYLOAD2DEVICE_PARSER_STATE_PROTO
};

static const char* HINTS_RESERVED_WORDS[] {
    "",                     // PAYLOAD2DEVICE_PARSER_STATE_START
    "",                     // PAYLOAD2DEVICE_PARSER_STATE_COMMAND
    " payload",             // PAYLOAD2DEVICE_PARSER_STATE_ADDRESS
    " fport",               // PAYLOAD2DEVICE_PARSER_STATE_FPORT
    " payload",             // PAYLOAD2DEVICE_PARSER_STATE_PAYLOAD
    " fopts",               // PAYLOAD2DEVICE_PARSER_STATE_FOPTS
    " at",// PAYLOAD2DEVICE_PARSER_STATE_TIME
    " proto"               // PAYLOAD2DEVICE_PARSER_STATE_PROTO
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
                    return (char *) HINTS_VALUE[parser.lastSendOption];
            }
        } else {
            *color = 32;
            *bold = 1;
            return (char *) HINTS_RESERVED_WORDS[parser.lastSendOption];
        }
    }
    return nullptr;
}

static const char *CMDS[] {
    "ping",     // 0
    "send",     // 1
    "quit"      // 2
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

static void run(
    bool *running
) {
    while(*running) {
        // std::cout << "--" << std::endl;
        sleep(10);
    }
}

// Asynchronous mode using the multiplexing API
static int cliOutput(
    char const* prompt,
    const char *historyFileName
) {
    linenoiseSetCompletionCallback(completionHook);
    linenoiseSetHintsCallback(hintsHook);
    linenoiseHistoryLoad(historyFileName);

    bool running = true;
    std::thread t(run, &running);

    while (true) {
        struct linenoiseState ls;
        char buf[1024];
        char *l;
        linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), prompt);
        while (true) {
            fd_set readFds;
            struct timeval tv;

            FD_SET(ls.ifd, &readFds);
            tv.tv_sec = 1; // 1 sec timeout
            tv.tv_usec = 0;

            int r = select(ls.ifd + 1, &readFds, nullptr, nullptr, &tv);
            l = nullptr;
            if (r == -1) {
                return ERR_CODE_SELECT;
            } else if (r) {
                l = linenoiseEditFeed(&ls);
                // A NULL return means: line editing is continuing. Otherwise, the user hit enter or stopped editing (CTRL+C/D)
                if (l != linenoiseEditMore)
                    break;
            } else {
                // Timeout occurred
            }

        }
        linenoiseEditStop(&ls);

        if (l) {
            linenoiseHistoryAdd(l);
            auto cmd = processInput(l);
            free(l);
            if (cmd == PAYLOAD2DEVICE_COMMAND_QUIT)
                break;
        }
    }
    linenoiseHistorySave(historyFileName);
    running = false;
    t.join();
}

static int cliNoOutput(
    char const* prompt,
    const char *historyFileName
) {

    linenoiseSetCompletionCallback(completionHook);
    linenoiseSetHintsCallback(hintsHook);
    linenoiseHistoryLoad(historyFileName);

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
    linenoiseHistorySave(historyFileName);
}

int main (
    int argc,
    char** argv
) {
    char const* prompt = "tlns> ";
    const std::string historyFileName(getHomeDir() + "/tlns-app-cli.history");
    // return cliNoOutput(prompt, historyFileName.c_str());
    return cliOutput(prompt, historyFileName.c_str());
}
