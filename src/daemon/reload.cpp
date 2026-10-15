#include "server.hpp"
#include <utils.hpp>

static void handleRemovedPrograms(Configs &configs, const Configs &newConfigs) {
    for (auto &[name, config] : configs.m_configs) {
        if (newConfigs.m_configs.count(name))
            continue;
        config.setConfigToExile();
        for (auto &program : config.programs)
            config.requestStop(program, -1);
    }
}

static int handleNewPrograms(Configs &configs, const Configs &newConfigs) {

    for (auto &[name, config] : newConfigs.m_configs) {
        if (configs.m_configs.count(name))
            continue;

        auto [it, inserted] = configs.m_configs.emplace(name, config);
        if (inserted && it->second.shouldAutostart()) {
            if (it->second.startAllPrograms() == -1) {
                return -1;
            }
        }
    }
    return 1;
}

static void handleModifiedPrograms(Configs &configs, const Configs &newConfigs) {
    for (const auto &[name, newConfig] : newConfigs.m_configs) {
        auto it = configs.m_configs.find(name);
        if (it == configs.m_configs.end())
            continue;

        ProgramConfig &oldConfig = it->second;
        if (ProgramConfig::areSettingsEqual(oldConfig, newConfig))
            continue;

        ProgramConfig::copySettings(newConfig, oldConfig);

        const bool autostart = oldConfig.getAutoStart();
        const size_t n = oldConfig.programs.size();

        oldConfig.openRedirection();
        for (size_t i = 0; i < n; ++i) {
            program &p = oldConfig.programs[i];
            if (p.exiled)
                continue;
            if (oldConfig.requestStop(p, -1) || p.state == State::Stopping)
                p.restarting = autostart;
            else if (autostart)
                oldConfig.startProgram(p);
        }
        if (autostart)
            oldConfig.scaleToNumprocs();
    }
}

int handleReload(int client_fd, Configs &configs) {
    Configs newConfigs;
    int result = ini_parse(configs.server.getConfigPath().c_str(), handler, &newConfigs);

    if (result < 0) {
        reply(client_fd, "Could not open config file\n");
        return ABORTED;
    } else if (result > 0) {
        reply(client_fd, "Parse error on line " + std::to_string(result) + "\n");
        return ABORTED;
    }
    handleRemovedPrograms(configs, newConfigs);
    if (handleNewPrograms(configs, newConfigs) == -1) {
        return SHUTDOWN;
    }
    handleModifiedPrograms(configs, newConfigs);

    reply(client_fd, "Reloaded config file\n");
    return 1;
}
