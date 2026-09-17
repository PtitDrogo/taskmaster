#include "server.hpp"


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

/*
Exemple:
[program:web]
command = python app.py
autostart = true
numprocs = 3

Section = [program:web] //Same for all 3 key/value pair
name = "command"
value = "python app.py"

*/

static int handler(void *user, const char *section, const char *name, const char *value) {
    auto *cfg = static_cast<Configs *>(user);
    std::string sect(section), setting(name), val(value);
    int err = 1;
    if (sect.rfind("program:", 0) == 0) {
        std::string progname = sect.substr(8); // strip "program:"
        ProgramConfig &pc = cfg->programs[progname];
        err = pc.parseSetting(setting, val);
    } else if (sect == "unix_http_server" || sect == "inet_http_server" || sect == "supervisord") {
        err = cfg->server.parseSetting(setting, val);
    } else if (sect.rfind("rpcinterface:", 0) == 0) {
        // Idk what that is I dont think we need to handle that
    }
    return err;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Error: argument expected" << std::endl;
    }
    Configs configs;

    int result = ini_parse(argv[1], handler, &configs);
    if (result < 0) {
        std::cerr << "Could not open config file\n";
        return 1;
    } else if (result > 0) {
        std::cerr << "Parse error on line " << result << "\n";
        return 1;
    }

    std::cout << "I am the Daemon/Server !" << std::endl;
    configs.printSettings();
    return 0;
}
