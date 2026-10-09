#pragma once

#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <string>

#include "ClientConfig.hpp"
#include "ini.h"

#define HELP_STRING "help - status - start - stop\nrestart - reload - shutdown"

void readResponseAndPrint(int fd);
char **command_completion(const char *text, int start, int end);