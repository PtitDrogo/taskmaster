#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "ProgramConfig.hpp"
#include <string>

class ProgramConfig;
struct program;
enum class State;

class Logger {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRIT };

    static Logger &get();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    int open(const std::string &path);
    int writePid(const std::string &path);
    void close();

    void log(Level lvl, const std::string &msg);

    void debug(const std::string &m) { log(DEBUG, m); }
    void info(const std::string &m) { log(INFO, m); }
    void warn(const std::string &m) { log(WARN, m); }
    void error(const std::string &m) { log(ERROR, m); }
    void crit(const std::string &m) { log(CRIT, m); }
    void printState(State state, program p, ProgramConfig cfg);

  private:
    Logger() = default;
    ~Logger();

    static const char *levelName(Level l);
    static std::string timestamp();
    void advancedLogs(Level lvl, const std::string &msg);

    int fd = -1;
    std::string path;
};

#endif