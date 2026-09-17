#include "client.hpp"


static int handler(void *user, const char *section, const char *name, const char *value) {
    auto *cfg = static_cast<ClientConfig *>(user);
    std::string sect(section), setting(name), val(value);
    int err = 1;
    if (sect == "supervisorctl") {
        err = cfg->parseSetting(setting, val);
    }
    return err;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Error: argument expected" << std::endl;
    }

    ClientConfig config;

    int result = ini_parse(argv[1], handler, &config);
    if (result < 0) {
        std::cerr << "Could not open config file\n";
        return 1;
    } else if (result > 0) {
        std::cerr << "Parse error on line " << result << "\n";
        return 1;
    }

    config.printSettings();

    char* line;
    while ((line = readline("supervisor> ")) != nullptr) {
        std::string input(line);
        if (!input.empty()) {
            add_history(line);
        }
        free(line);

        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;

        if (cmd == "help") {
            std::cout << "HELP - SHUTDOWN - OTHER STUFF" << std::endl;
        }
        else if (cmd.empty()) continue;
        else std::cout << "Unknown command: " << cmd << "\n";
    }


    clear_history(); //This is the clear history for the shell.
    return 0;
}