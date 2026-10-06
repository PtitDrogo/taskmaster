#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>

#include "ProgramConfig.hpp"
#include "ServerConfig.hpp"
#include "ini.h"

#define SHUTDOWN -2
#define CLIENT_DISCONNECT -3

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

    program *findByPid(pid_t pid) {
        for (auto &[name, cfg] : programs) {
            for (auto &p : cfg.programs) {
                if (p.pid == pid)
                    return &p;
            }
        }
        return nullptr;
    }

    void killTimedOutPrograms() {
        time_t now = time(nullptr);
        for (auto &[name, cfg] : programs)
            for (auto &p : cfg.programs)
                if (p.state == State::Stopping && now > p.kill_deadline)
                    kill(-p.pid, SIGKILL);
    }
};

int dropPrivileges(const std::string &user);
int handleCommands(int client_fd, std::string fullCmd, const Configs &configs);
