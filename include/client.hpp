#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <string>

#include "ClientConfig.hpp"
#include "ini.h"

#define HELP_STRING "status - start - stop - restart\nreload - attach - shutdown"

void readResponseAndPrint(int fd);
char **command_completion(const char *text, int start, int end);
void attachSession(int fd);

#endif