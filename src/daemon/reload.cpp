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
    for (auto &keyValue : configs.programs) {
        try {
            ProgramConfig tmp = newConfigs.programs.at(keyValue.first);
        } catch (const std::out_of_range &e) {
            ProgramConfig &config = keyValue.second;
            config.setProgramToExile();
            for (auto &program : config.programs) {
                config.requestStop(program, -1);
            }
        }
    }
}

void handleReload(int client_fd, Configs &configs) {
    // Read and parse the new config file
    Configs newConfig;
    int result = ini_parse(configs.server.getConfigPath().c_str(), handler, &newConfig);
    if (result < 0) {
        reply(client_fd, "Could not open config file\n");
        return;
    } else if (result > 0) {
        reply(client_fd, "Parse error on line " + std::to_string(result) + "\n");
        return;
    }
    handleRemovedPrograms(configs, newConfig);
    reply(client_fd, "Reloaded config file\n");
    // Compare before and after

    // Stop non existant processes, they shouldnt even show when typing status
    // Start the new processes.
    // Restart modified processes.

    // Maybe its smarter to just flip stuff in Program and let the tick function take care of it.

    // should_reload
    // should_die_for_real

    // Maybe just the act of starting brand new processes is the thing i should do here ?

    return;
}
