#pragma once

#include <iostream>
#define SOCK_PATH "/tmp/supervisor.sock"

class ServerConfig {
  private:
    std::string user;
    std::string sockfile; // [unix_http_server]
    std::string logfile = "taskmasterd.log";
    std::string pidfile = "taskmasterd.pid";
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
    std::string getLogFile(){ return this->logfile;}
    std::string getPidFile(){ return this->pidfile;}
    void setConfigPath(std::string path) { m_config_file_path = path; }
    const std::string getUser() { return user; }
};

class Logger {
public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRIT };

    static Logger& get();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    int open(const std::string& path);
    int writePid(const std::string& path);
    void close();

    void log(Level lvl, const std::string& msg);

    void debug(const std::string& m) { log(DEBUG, m); }
    void info (const std::string& m) { log(INFO,  m); }
    void warn (const std::string& m) { log(WARN,  m); }
    void error(const std::string& m) { log(ERROR, m); }
    void crit (const std::string& m) { log(CRIT,  m); }

private:
    Logger() = default;
    ~Logger();

    static const char* levelName(Level l);
    static std::string timestamp();

    int         fd = -1;
    std::string path;
};