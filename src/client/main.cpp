#include "client.hpp"
#include "ini.h"
#include "taskmaster.hpp"

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

    return 0;
}