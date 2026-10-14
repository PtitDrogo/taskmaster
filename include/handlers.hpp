#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include <poll.h>
#include <sys/types.h>
#include <vector>

#include "server.hpp"

int handler(void *user, const char *section, const char *name, const char *value);
void handleDeadProcesses(Configs &configs);
void readAttachedClients(std::vector<pollfd> &fds, std::vector<pollfd> &pfds,
                         std::vector<AttachedClients> &attachedClients);
void handleAttachedInput(program &p, const char *buf, ssize_t n);
void readClientInputs(std::vector<pollfd> &fds, Configs &configs);

#endif