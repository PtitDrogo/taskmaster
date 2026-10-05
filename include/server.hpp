#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>

#include "ServerConfig.hpp"
#include "ProgramConfig.hpp"
#include "ini.h"

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
};

int dropPrivileges(const std::string &user);
void handleCommands(int client_fd, std::string cmd, const Configs &configs);
void handleStatusCmd(int client_fd, const Configs &configs);
