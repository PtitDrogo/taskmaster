#pragma once

#include <poll.h>
#include <vector>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>

#include "ProgramConfig.hpp"
#include "ServerConfig.hpp"
#include "ini.h"


#define SHUTDOWN -2
#define CLIENT_DISCONNECT -3
#define ABORTED -4

struct Configs {
    ServerConfig server;
    std::map<std::string, ProgramConfig> programs;

    void printSettings() const {
        server.printSettings();
        for (auto &[section, cfg] : programs) {
            std::cout << "Section: " << section << "\n";
            cfg.printSettings();
        }
    }

    std::pair<ProgramConfig *, program *> findByPid(pid_t pid) {
        if (pid <= 0)
            return {nullptr, nullptr};
        for (auto &[name, config] : programs) {
            for (auto &program : config.programs) {
                if (program.pid == pid)
                    return {&config, &program};
            }
        }
        return {nullptr, nullptr};
    }

    void forgetClient(int fd) {
        for (auto &[name, cfg] : programs)
            for (auto &p : cfg.programs)
                if (p.waiting_client == fd)
                    p.waiting_client = -1;
    }
};

int dropPrivileges(const std::string &user);
int handleCommands(int client_fd, std::string fullCmd, Configs &configs);
void cleanup(std::vector<pollfd> &fds, Configs &configs);
