#include <cstring>
#include <readline/history.h>
#include <readline/readline.h>

const char *commands[] = {"status", "start", "stop", "restart", "reload", "shutdown", "help", nullptr};

static char *command_generator(const char *text, int state) {
    static int index;
    if (state == 0)
        index = 0;

    while (commands[index]) {
        const char *cmd = commands[index++];

        if (strncmp(cmd, text, strlen(text)) == 0)
            return strdup(cmd);
    }

    return nullptr;
}

char **command_completion(const char *text, int start, int end) {
    (void)end;

    if (start == 0)
        return rl_completion_matches(text, command_generator);

    return nullptr;
}