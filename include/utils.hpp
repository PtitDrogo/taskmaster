#pragma once

#include <string>
#include <unistd.h>

void reply(int fd, const std::string &msg);
bool notifyDiscord(const std::string &message);

constexpr char DETACH_KEY = 0x1d; // Ctrl-]