#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include "ProgramConfig.hpp"
#include <iostream>
#define SOCK_PATH "/tmp/supervisor.sock"

class ServerConfig {
  private:
    std::string user;
    std::string sockfile;
    std::string logfile = "taskmasterd.log";
    std::string pidfile = "taskmasterd.pid";
    bool nodaemon = false;
    std::string serverurl;
    std::string m_config_file_path;

  public:
    ServerConfig();
    ~ServerConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    static int startDaemonServer();
    std::string getConfigPath() const { return m_config_file_path; }
    std::string getLogFile() { return this->logfile; }
    std::string getPidFile() { return this->pidfile; }
    void setConfigPath(std::string path) { m_config_file_path = path; }
    const std::string getUser() { return user; }
};

#endif