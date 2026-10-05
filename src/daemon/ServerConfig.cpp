#include "ServerConfig.hpp"
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

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

int ServerConfig::startDaemonServer() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    unlink(SOCK_PATH); // remove old socket file if it exists

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