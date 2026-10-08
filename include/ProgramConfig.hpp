#pragma once

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

enum class State { Stopped, Starting, Running, Backoff, Stopping, Exited, Fatal };

struct program {
    pid_t pid = -1;
    State state = State::Starting;
    time_t start_time = 0;
    int curr_retries = 0; // failed starts in a row
    time_t backoff_until = 0;
    bool killing = false; // a stop was requested
    time_t kill_deadline = 0;
    int waiting_client = -1;
    bool restarting = false;
};

// This hold the config of every created program.
class ProgramConfig {
  private:
    std::string name;
    std::string cmd;
    int numprocs = 1; // How many process it should make
    bool autostart = true;
    int startsecs = 1;
    enum class AutoRestart { Always, Never, Unexpected } autorestart = AutoRestart::Unexpected;
    std::vector<int> exitcodes = {0};
    int startretries = 3;
    int stopsignal = SIGINT;
    int stoptime = 10;
    bool discard_stdout = false, discard_stderr = false;
    std::string stdout_logfile = "", stderr_logfile = "";
    std::map<std::string, std::string> envMap;
    std::vector<std::string> env;
    std::vector<char *> envptr;
    std::string workingdir;
    mode_t umask = 022; // Octal value, this is about setting the files perimission this program will create.
    int fdout = 0;
    int fderr = 0;

  public:
    std::vector<program> programs;
    ProgramConfig(/* args */);
    ~ProgramConfig();
    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    bool requestStop(program &p, int client_fd);
    bool shouldAutostart() const { return autostart; }
    void startProgram(program &p, int i);
    int startAllPrograms();
	
    void tick(program &p, time_t now);
    void onExit(program &p, int status);
    int parseSignals(std::string signal);
    
	//environment
	int addEnvironnement(const std::string &value);
    void fillEnvp(int i);
    char **getEnvp();

	//redirection
    int openLog(const std::string &path, const std::string &logfile);
    void redirectFiles();
    int openRedirection();
	int generateRandomFile(const std::string &logfile);
};
