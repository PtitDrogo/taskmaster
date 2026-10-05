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

  public:
    ServerConfig(/* args */);
    ~ServerConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    static int startDaemonServer();
};