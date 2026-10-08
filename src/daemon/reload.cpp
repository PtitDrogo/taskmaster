#include "server.hpp"
#include <utils.hpp>

// I have to parse the new config file again.
// I have to detect which config changed.

// Unchanged config arent touched.
// New processes must be started
// Program with a modified config must be reloaded
// Program removed from the config, we stop them.

// I cant think of a super super smart way to do it. So lets do it in passes.

// First we pass the new config and we generate a new Configs object.
// We set programs in the existing config to be deleted for real.
static void handleRemovedPrograms(Configs &configs, const Configs &newConfigs) {
    for (auto &[name, config] : configs.programs) {
        if (newConfigs.programs.count(name))
            continue;
        config.setProgramToExile();
        for (auto &program : config.programs)
            config.requestStop(program, -1);
    }
}

static int handleNewPrograms(Configs &configs, const Configs &newConfigs) {

    for (auto &[name, config] : newConfigs.programs) {
        if (configs.programs.count(name))
            continue;

        // We add new programs to configs and then we start them, simple as
        auto [it, inserted] = configs.programs.emplace(name, config);
        if (inserted && it->second.shouldAutostart()) {
            if (it->second.startAllPrograms() == -1) {
                return -1;
            }
        }
    }
    return 1;
}
static void handleModifiedPrograms(Configs &configs, const Configs &newConfigs) {
    for (const auto &[name, newConfig] : newConfigs.programs) {
        auto it = configs.programs.find(name);
        if (it == configs.programs.end())
            continue;

        ProgramConfig &oldConfig = it->second;
        if (ProgramConfig::areSettingsEqual(oldConfig, newConfig))
            continue;

        ProgramConfig::copySettings(newConfig, oldConfig);
        for (auto &program : oldConfig.programs) {
            oldConfig.requestStop(program, -1);
            if (oldConfig.getAutoStart()) {
                program.restarting = true;
            }
        }
    }
}

int handleReload(int client_fd, Configs &configs) {
    // Read and parse the new config file
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
