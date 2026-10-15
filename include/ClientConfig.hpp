#ifndef CLIENT_CONFIG_HPP
#define CLIENT_CONFIG_HPP

#include <iostream>

class ClientConfig {
  private:
    std::string serverurl;
    std::string username;
    std::string password;
    std::string prompt = "supervisor";

  public:
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    static int startClient();
};

#endif