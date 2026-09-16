#include "server.hpp"

ServerConfig::ServerConfig(/* args */) {}

ServerConfig::~ServerConfig() {}

void ServerConfig::printSettings() const {
    std::cout << "[unix_http_server]\n"
              << "  sockfile: " << sockfile << "\n"
              << "[supervisord]\n"
              << "  logfile: " << logfile << "\n"
              << "  pidfile: " << pidfile << "\n"
              << "  nodaemon: " << std::boolalpha << nodaemon << "\n"
              << "[supervisorctl]\n"
              << "  serverurl: " << serverurl << "\n";
}

int ServerConfig::parseSetting(const std::string &setting, const std::string &value) {
    if (setting == "file") {
        sockfile = value;
    } else if (setting == "logfile") {
        logfile = value;
    } else if (setting == "pidfile") {
        pidfile = value;
    } else if (setting == "nodaemon") {
        nodaemon = (value == "true");
    } else if (setting == "serverurl") {
        serverurl = value;
    } else {
        std::cerr << "Unknown setting: " << setting << " in [supervisord]\n";
        return 0;
    }

    return 1;
}
