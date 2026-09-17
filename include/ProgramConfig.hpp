#pragma once

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct program {
    pid_t pid;
    std::string status; // Probably en enum later on.
};

// This hold the config of every created program.
struct ProgramConfig {
  private:
    std::string name;
    std::string cmd;
    int numprocs = 1; // How many process it should make
    bool autostart = true;
    int startsecs = 1;
    enum class AutoRestart { Always, Never, Unexpected } autorestart = AutoRestart::Unexpected;
    std::vector<int> exitcodes = {0};
    int starttime = 1;
    int startretries = 3;
    int stopsignal = SIGTERM;
    int stoptime = 10;
    bool discard_stdout = false, discard_stderr = false;
    std::string stdout_logfile, stderr_logfile;
    std::map<std::string, std::string> env;
    std::string workingdir;
    mode_t umask = 022; // Octal value, this is about setting the files perimission this program will create.

  public:
    std::vector<program> programs;
    ProgramConfig(/* args */);
    ~ProgramConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    int createProgram(const char *cmd);
    int startAllPrograms(const std::map<std::string, ProgramConfig> &programs);
};
