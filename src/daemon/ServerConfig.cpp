#include "ServerConfig.hpp"
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

ServerConfig::ServerConfig(/* args */) {}

ServerConfig::~ServerConfig() { unlink(this->pidfile.data()); }

void ServerConfig::printSettings() const {
    std::cout << "  user: " << user << "\n"
              << "  logfile: " << logfile << "\n"
              << "  config_path: " << m_config_file_path << "\n";
}

int ServerConfig::parseSetting(const std::string &setting, const std::string &value) {
    if (setting == "user") {
        if (!user.empty()) {
            std::cerr << "Can't set 'user' twice in [supervisord]\n";
        }
        user = value;
    } else if (setting == "logfile") {
        if (logfile != "taskmasterd.log") {
            std::cerr << "Can't set 'logfile' twice in [supervisord]\n";
        }
        logfile = value;
    } else if (setting == "pidfile") {
        pidfile = value;
    } else {
        std::cerr << "Unknown setting: " << setting << " in [supervisord]\n";
        return 1;
    }

    return 1;
    // return 0;
}

int ServerConfig::startDaemonServer() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    unlink(SOCK_PATH);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return -1;
    }
    return server_fd;
}