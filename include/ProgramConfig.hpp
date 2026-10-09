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
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

enum class State { Stopped, Starting, Running, Backoff, Stopping, Exited, Fatal };

struct program {
    pid_t pid = -1;
    State state = State::Starting;
    time_t start_time = 0;
    int curr_retries = 0; // failed starts in a row
    time_t backoff_until = 0;
    bool killing = false; // a stop was requested
    time_t kill_deadline = 0;
    int waiting_client = -1; // Technically this should be a vector of clients.
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
    std::string workingdir = "";
    mode_t umask = 022; // Octal value, this is about setting the files perimission this program will create.
    
    // runtime
    bool m_exiled_program = false; // mtg reference, stronger than destroy, since it wont show in logs.
    int fdout = 0;
    int fderr = 0;

  public:
    std::vector<program> programs;

    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    bool requestStop(program &p, int client_fd);
    bool shouldAutostart() const { return autostart; }
    void startProgram(program &p);
    int startAllPrograms();
	
    void tick(program &p, time_t now);
    void onExit(program &p, int status);
    int parseSignals(std::string signal);
    
    std::string getName() { return name; }
    void setName(std::string newName) { name = newName; }
    void setProgramToExile() { m_exiled_program = true; }
    bool isExiled() const { return m_exiled_program; }
    bool getAutoStart() const { return autostart; }

    static bool areSettingsEqual(const ProgramConfig &a, const ProgramConfig &b);
    static void copySettings(const ProgramConfig &src, ProgramConfig &dest);
	
    //environment
	int addEnvironnement(const std::string &value);
    void fillEnvp();
    char **getEnvp();

	//redirection
    int openLog(const std::string &path, const std::string &identifier);
    void redirectFiles();
    int openRedirection();
    void removeOlderLogFile(const std::string &identifier);
	int generateRandomFile(const std::string &identifier);
};
