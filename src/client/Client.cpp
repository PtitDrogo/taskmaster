#include "client.hpp"

ClientConfig::ClientConfig(/* args */) {}

ClientConfig::~ClientConfig() {}

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