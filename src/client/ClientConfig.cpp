
#include "ClientConfig.hpp"
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/supervisor.sock"

ClientConfig::ClientConfig(/* args */) {}

ClientConfig::~ClientConfig() { std::cout << "Im running the destructor for clear history" << std::endl; }

void ClientConfig::printSettings() const {
    std::cout << "[supervisorctl]\n"
              << "  serverurl: " << serverurl << "\n"
              << "  username: " << username << "\n"
              << "  password: " << (password.empty() ? "" : "****") << "\n"
              << "  prompt: " << prompt << "\n";
}

int ClientConfig::parseSetting(const std::string &setting, const std::string &value) {
    if (setting == "serverurl") {
        serverurl = value;
    } else if (setting == "username") {
        username = value;
    } else if (setting == "password") {
        password = value;
    } else if (setting == "prompt") {
        prompt = value;
    } else {
        std::cerr << "Unknown setting: " << setting << " in [supervisorctl]\n";
        return 0;
    }

    return 1;
}

int ClientConfig::startClient() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return -1;
    }
    return fd;
}