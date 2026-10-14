#include "ServerConfig.hpp"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

Logger &Logger::get() {
    static Logger instance;
    return instance;
}

Logger::~Logger() { this->close(); }

int Logger::open(const std::string &path) {
    if (this->fd != -1)
        ::close(this->fd);
    this->fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (this->fd < 0) {
        std::cout << "Error:" << path << " " << strerror(errno) << std::endl;
        return -1;
    }
    this->path = path;
    return this->fd;
}

void Logger::close() {
    if (this->fd != -1) {
        ::close(this->fd);
        this->fd = -1;
    }
}

int Logger::writePid(const std::string &path) {
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        std::cout << "Error:" << path << " " << strerror(errno) << std::endl;
        return -1;
    }
    std::string data = std::to_string(getpid()) + "\n";

    if (write(fd, data.c_str(), data.size()) < 0) {
        std::cout << "Error:" << path << " " << strerror(errno) << std::endl;
        return -1;
    }
    return 1;
}

const char *Logger::levelName(Level l) {
    switch (l) {
    case DEBUG:
        return "DEBG";
    case INFO:
        return "INFO";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERRO";
    case CRIT:
        return "CRIT";
    }
    return "????";
}

void Logger::printState(State state, program p, ProgramConfig cfg){
    switch (state) {
        case State::Starting:
            this->log(Level::INFO, "spawned: 'noname' with pid " +  std::to_string(p.pid));
            break;
        case State::Running:
            this->log(Level::INFO, "success: noname entered RUNNING state, process has stayed up for > than " + std::to_string(cfg.getStartSecs()) + " seconds (startsecs)");
            break;
        case State::Stopped:{
            const char *abbr = sigabbrev_np(p.exitsig);
            std::string name = abbr ? std::string("SIG") + abbr : "signal " + std::to_string(p.exitsig);
            this->log(Level::WARN, "stopped: noname (terminated by " + name + ")");
            break;
        }
        case State::Exited:
            this->log(Level::WARN, "exited: noname (exit status " + std::to_string(p.exitcode) + "; not expected)");
            break;
        case State::Fatal:
            this->log(Level::INFO, "gave up: nonname entered FATAL state, too many start retries too quickly");
            break;    
        default:
            break;
    }
}

// Format: 2026-10-10 14:32:01,123
std::string Logger::timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);

    char buf[40];
    size_t n = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(buf + n, sizeof(buf) - n, ",%03d", (int)(tv.tv_usec / 1000));
    return buf;
}

void Logger::log(Level lvl, const std::string &msg) {
    if (this->fd == -1)
        return;

    std::string line = timestamp() + " " + levelName(lvl) + " " + msg + "\n";
    ssize_t r = ::write(this->fd, line.c_str(), line.size());
    (void)r;
}