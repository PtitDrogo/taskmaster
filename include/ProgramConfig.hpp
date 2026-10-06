#pragma once

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

enum class State { Stopped, Starting, Running, Backoff, Stopping, Exited, Fatal };

struct program {
    pid_t pid = -1;
    State state = State::Starting;
    time_t start_time = 0;
    int currRetries = 0;  // failed starts in a row
    time_t backoff_until = 0;
    bool killing = false; // a stop was requested
    time_t kill_deadline = 0;
    int waiting_client = -1;
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
    void startProgram(program &p);
    bool requestStop(program &p, int client_fd);
    bool shouldAutostart() const { return autostart; }
    int startAllPrograms();

    void tick(program &p, time_t now);
    void onExit(program &p, int status);
};
