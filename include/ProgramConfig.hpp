#ifndef PROGRAM_CONFIG_HPP
#define PROGRAM_CONFIG_HPP

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define SCROLLBACK_MAX 16384

namespace fs = std::filesystem;

enum class State { Stopped, Starting, Running, Backoff, Stopping, Exited, Fatal };

struct program {
    std::string name;
    pid_t pid = -1;
    int exitcode = -1;
    int exitsig = -1;
    State state = State::Starting;
    time_t start_time = 0;
    int curr_retries = 0; // failed starts in a row
    time_t backoff_until = 0;
    bool killing = false; // a stop was requested
    time_t kill_deadline = 0;
    int waiting_client = -1; // Technically this should be a vector of clients.
    bool restarting = false;

    int master_fd = -1;
    int err_fd = -1;
    int attached_client = -1;
    std::string scrollback;
    bool exiled = false;

    void print() {
        std::cout << std::boolalpha << "program{pid=" << pid << " state=" << static_cast<int>(state)
                  << " start_time=" << start_time << " curr_retries=" << curr_retries
                  << " backoff_until=" << backoff_until << " killing=" << killing << " kill_deadline=" << kill_deadline
                  << " waiting_client=" << waiting_client << " restarting=" << restarting << "}\n";
    }
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
    int stopsignal = SIGTERM;
    int stoptime = 10;
    int stopwaitsecs = 10;
    bool discard_stdout = false, discard_stderr = false;
    std::string stdout_logfile = "", stderr_logfile = "";
    std::map<std::string, std::string> envMap;
    std::vector<std::string> env;
    std::vector<char *> envptr;
    std::string workingdir = "";
    mode_t m_umask = 022; // Octal value, this is about setting the files perimission this program will create.
    std::string user;

    // runtime
    bool m_exiled_program = false; // mtg reference, stronger than destroy, since it wont show in logs.
    int fdout = 0;
    int fderr = 0;

    // Parsing
    std::set<std::string> m_seen;
    template <typename T> bool setOnce(const std::string &setting, T &field, T value) {
        if (!m_seen.insert(setting).second) {
            std::cerr << "Can't set '" << setting << "' twice in [" << name << "]\n";
            return false;
        }
        field = std::move(value);
        return true;
    }

    bool isNameTaken(std::string name);

  public:
    std::vector<program> programs;

    void printSettings() const;
    int parseSetting(const std::string &setting, const std::string &value);
    bool requestStop(program &p, int client_fd);
    bool shouldAutostart() const { return autostart; }
    void startProgram(program &p);
    int startAllPrograms();
    void scaleToNumprocs();

    void tick(program &p, time_t now);
    void onExit(program &p, int status);
    int parseSignals(std::string signal);
    void setState(State state, program &p);

    //getter
    std::string getName() const { return name; }
    bool getAutoStart() const { return autostart; }
    int getStartSecs() const;
    char **getEnvp();

    //setter
    void setName(std::string newName) { name = newName; }
    void setProgramName(program &p);
    void setConfigToExile() { m_exiled_program = true; }
    bool isExiled() const { return m_exiled_program; }

    void onMasterReadable(program &p);
    void onErrReadable(program &p);
    void closeMaster(program &p);
    void closeErr(program &p);

    static bool areSettingsEqual(const ProgramConfig &a, const ProgramConfig &b);
    static void copySettings(const ProgramConfig &src, ProgramConfig &dest);

    // environment
    int addEnvironnement(const std::string &value);
    void fillEnvp();

    // redirection
    int openLog(const std::string &path, const std::string &identifier);
    void redirectFiles();
    int openRedirection();
    void removeOlderLogFile(const std::string &identifier);
    int generateRandomFile(const std::string &identifier);

    void reapExiled();
};

int dropPrivilegesProgram(const std::string &user);

#endif