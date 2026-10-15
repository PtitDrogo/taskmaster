#ifndef SERVER_HPP
#define SERVER_HPP

#include <functional>
#include <iostream>
#include <map>
#include <poll.h>
#include <syslog.h>
#include <unordered_map>
#include <vector>

#include "ProgramConfig.hpp"
#include "ServerConfig.hpp"
#include "ini.h"

#define SHUTDOWN -2
#define CLIENT_DISCONNECT -3
#define ABORTED -4

struct AttachedClients {
    ProgramConfig *cfg;
    program *p;
    bool isErr;
};

struct Configs {
    ServerConfig server;
    std::map<std::string, ProgramConfig> m_configs;

    void printSettings() const {
        server.printSettings();
        for (auto &[section, cfg] : m_configs) {
            std::cout << "Section: " << section << "\n";
            cfg.printSettings();
        }
    }

    std::pair<ProgramConfig *, program *> findByPid(pid_t pid) {
        if (pid <= 0)
            return {nullptr, nullptr};
        for (auto &[name, config] : m_configs) {
            for (auto &program : config.programs) {
                if (program.pid == pid)
                    return {&config, &program};
            }
        }
        return {nullptr, nullptr};
    }

    std::pair<ProgramConfig *, program *> findByProgramName(const std::string &name) {
        for (auto &config : m_configs) {
            for (auto &prog : config.second.programs) {
                if (prog.name == name) {
                    return {&config.second, &prog};
                }
            }
        }
        return {nullptr, nullptr};
    }

    program *findAttached(int client_fd) {
        for (auto &[name, cfg] : m_configs)
            for (auto &p : cfg.programs)
                if (p.attached_client == client_fd)
                    return &p;
        return nullptr;
    }

    void forgetClient(int fd) {
        for (auto &[name, cfg] : m_configs)
            for (auto &p : cfg.programs) {
                if (p.waiting_client == fd)
                    p.waiting_client = -1;
                if (p.attached_client == fd)
                    p.attached_client = -1; // new
            }
    }

    void startAllConfigs() {
        for (auto &[name, cfg] : this->m_configs) {
            if (cfg.shouldAutostart())
                cfg.startAllPrograms();
        }
    }

    void addAttachedFds(std::vector<pollfd> &pfds, std::vector<AttachedClients> &attachedClients) {
        for (auto &[name, cfg] : this->m_configs) {
            for (auto &p : cfg.programs) {
                if (p.master_fd != -1) {
                    pfds.push_back({p.master_fd, POLLIN, 0});
                    attachedClients.push_back({&cfg, &p, false});
                }
                if (p.err_fd != -1) {
                    pfds.push_back({p.err_fd, POLLIN, 0});
                    attachedClients.push_back({&cfg, &p, true});
                }
            }
        }
    }

    void setConfigPath(std::string path) { server.setConfigPath(path); }

    void reapExiledConfigs() {
        for (auto it = m_configs.begin(); it != m_configs.end();) {
            ProgramConfig &cfg = it->second;

            bool allDead = std::all_of(cfg.programs.begin(), cfg.programs.end(), [](const program &p) {
                return p.state == State::Stopped || p.state == State::Exited || p.state == State::Fatal;
            });

            if (cfg.isExiled() && allDead) {
                for (auto &p : cfg.programs) {
                    cfg.closeMaster(p);
                    cfg.closeErr(p);
                }
                it = m_configs.erase(it);
            } else {
                ++it;
            }
        }
    }
};

int handleCommands(int client_fd, std::string fullCmd, Configs &configs);
void cleanup(std::vector<pollfd> &fds, Configs &configs);
int handleReload(int client_fd, Configs &configs);
int handler(void *user, const char *section, const char *name, const char *value);
int handleReload(int client_fd, Configs &configs);

int dropPrivilegesServer(const std::string &user);

#endif