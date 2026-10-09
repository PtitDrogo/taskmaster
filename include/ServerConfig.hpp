#pragma once

#include <iostream>
#define SOCK_PATH "/tmp/supervisor.sock"

class ServerConfig {
  private:
    std::string sockfile; // [unix_http_server]
    std::string logfile;
    std::string pidfile; // [supervisord]
    bool nodaemon = false;
    std::string serverurl; // [supervisorctl]
    std::string m_config_file_path;

  public:
    ServerConfig(/* args */);
    ~ServerConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    static int startDaemonServer();
    std::string getConfigPath() const { return m_config_file_path; }
    void setConfigPath(std::string path) { m_config_file_path = path; }
};


// void logMsgDaemon(std::string msg) {
//   static Logger;
  
//   Logger.printMsg();
// }