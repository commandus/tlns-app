#include "linenoise.h"

#include <cstring>
#include <cstdlib>
#include <iostream>

#include "lorawan/file-helper.h"
#include "lorawan/proto/payload2device/payload2device-parser.h"

static const char* examples[] = {
  "db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power",
  nullptr
};

void completionHook (
    char const* prefix,
    linenoiseCompletions* lc
)
{
    for (auto i = 0;  examples[i] != nullptr; i++) {
        if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
            linenoiseAddCompletion(lc, examples[i]);
        }
    }
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
    linenoiseInstallWindowChangeHandler();
    const std::string historyFileName (getHomeDir() + "/tlns-app-cli.history");

    linenoiseHistoryLoad(historyFileName.c_str());
    linenoiseSetCompletionCallback(completionHook);

    char const* prompt = "\x1b[1;32mtlns\x1b[0m> ";

    while (true) {
        char* l = linenoise(prompt);
        std::string line(l);
        free(l);
        if (processInput(line) == PAYLOAD2DEVICE_COMMAND_QUIT)
            break;
        linenoiseHistoryAdd(line.c_str());
    }
    linenoiseHistorySave(historyFileName.c_str());
    linenoiseHistoryFree();
}
