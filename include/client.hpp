#include <iostream>

class ClientConfig {
  private:
    std::string serverurl;
    std::string username;
    std::string password;
    std::string prompt = "supervisor"; // idk what that is actually.

  public:
    ClientConfig(/* args */);
    ~ClientConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
};
