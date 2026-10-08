#include "ProgramConfig.hpp"
#include <unordered_map>

ProgramConfig::ProgramConfig(/* args */) {}

ProgramConfig::~ProgramConfig() {}

void ProgramConfig::printSettings() const {
    std::cout << "Section: " << name << "\n"
              << "  command: " << cmd << "\n"
              << "  autostart: " << std::boolalpha << autostart << "\n"
              << "  numprocs: " << numprocs << "\n"
              << "  autorestart: " << static_cast<int>(autorestart) << "\n"
              << "  startsecs: " << startsecs << "\n"
              << "  startretries: " << startretries << "\n"
              << "  stopsignal: " << stopsignal << "\n"
              << "  stoptime: " << stoptime << "\n"
              << "  stdout logfile: " << stdout_logfile << "\n"
              << "  stderr logfile: " << stderr_logfile << "\n"
              << "  workingdir: " << workingdir << "\n"
              << "  umask: " << std::oct << umask << std::dec << "\n";

    if (!env.empty()) {
        std::cout << "  env:\n";
        for (auto &v: env)
            std::cout << "\t" << v << "\n";
    }
}

int ProgramConfig::parseSignals(std::string signal) {
    const std::unordered_map<std::string, int> signals = {{"HUP", SIGHUP},   {"INT", SIGINT},   {"QUIT", SIGQUIT},
                                                          {"KILL", SIGKILL}, {"TERM", SIGTERM}, {"USR1", SIGUSR1},
                                                          {"USR2", SIGUSR2}};

    auto it = signals.find(signal);
    if (it == signals.end())
        return -1;
    return it->second;
}

namespace{
    std::string stripQuotes(const std::string &s){
        if(s.size() >= 2 &&
           ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\''))){
            return s.substr(1, s.size() - 2);
        }
        return s;
    }
    
    bool isValidEnvName(const std::string &name){
        if(name.empty())
            return false;
        for(char c : name)
            if(!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
                return false;
        return true;
    }
}

int ProgramConfig::addEnvironnement(const std::string &value)
{
    std::size_t start = 0;

    while (start < value.size())
    {
        const std::size_t eq = value.find('=', start);
        if (eq == std::string::npos)
            return 1;

        bool inQuotes = false;
        std::size_t end = eq + 1;
        while (end < value.size() && (inQuotes || value[end] != ','))
        {
            if (value[end] == '"')
                inQuotes = !inQuotes;
            end++;
        }

        const std::string name = value.substr(start, eq - start);
        const std::string val  = stripQuotes(value.substr(eq + 1, end - eq - 1));

        if (!isValidEnvName(name))
            return -1;

        this->envMap[name] = val;
        start = end + 1;
    }
    return 0;
}

void ProgramConfig::fillEnvp()
{
    this->env.clear();
    this->envptr.clear();

    this->env.reserve(this->envMap.size());
    for (const auto &pair : this->envMap)
        this->env.push_back(pair.first + "=" + pair.second);

    // Pass 2: the vector is now final, so the pointers stay valid.
    this->envptr.reserve(this->env.size() + 1);
    for (auto &s : this->env)                    // non-const ref -> data() gives char *
        this->envptr.push_back(s.data());        // C++17; otherwise &s[0]
    this->envptr.push_back(nullptr);
}

int ProgramConfig::parseSetting(const std::string &setting, const std::string &value) {
    try {
        if (setting == "command") {
            cmd = value;
        } else if (setting == "autostart") {
            autostart = (value == "true");
        } else if (setting == "numprocs") {
            numprocs = std::stoi(value);
        } else if (setting == "startsecs") {
            startsecs = std::stoi(value);
        } else if (setting == "startretries") {
            startretries = std::stoi(value);
        } else if (setting == "stoptime") {
            stoptime = std::stoi(value);
        } else if (setting == "workingdir") {
            workingdir = value;
        } else if (setting == "stdout_discard") {
            discard_stdout = (value == "true");
        } else if (setting == "stderr_discard") {
            discard_stderr = (value == "true");
        } else if (setting == "stdout_logfile") {
            stdout_logfile = value;
            std::cout << "output " << stdout_logfile << std::endl;
        } else if (setting == "stderr_logfile") {
            stderr_logfile = value;
        } else if (setting == "umask") {
            umask = std::stoi(value, nullptr, 8);
        } else if (setting == "autorestart") {
            if (value == "always")
                autorestart = AutoRestart::Always;
            else if (value == "never")
                autorestart = AutoRestart::Never;
            else if (value == "unexpected")
                autorestart = AutoRestart::Unexpected;
        } else if (setting == "exitcodes") {
            exitcodes.push_back(std::stoi(value));
        } else if (setting == "startsecs") {
            startsecs = std::stoi(value);
        } else if (setting == "stopsignal") {
            int signal = parseSignals(value);
            if (signal != -1) {
                stopsignal = signal;
            }
        } else if (setting == "environment") {
            int err = addEnvironnement(value);
            if (err) {
                std::cerr << "Unknown setting: " << setting << " in [" << name << "]\n";
                return 0;
            }
        } else {
            std::cerr << "Unknown setting: " << setting << " in [" << name << "]\n";
            return 0;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 0;
    }

    return 1;
}

void ProgramConfig::redirectFiles(int fd_out, int fd_err){
    int devnull = open("/dev/null", O_RDONLY);
    dup2(devnull, STDIN_FILENO);
    close(devnull);

    dup2(fd_out, STDOUT_FILENO);
    close (fd_out);
    dup2(fd_err, STDERR_FILENO);
    close (fd_err);
}

char **ProgramConfig::getEnvp()
{
    return this->envptr.data();
}

int ProgramConfig::openLog(const std::string &path) {
    const char *p = path.empty() ? "/dev/null" : path.c_str();
    return open(p, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
}

// we gotta just call /bin/sh on everything
int ProgramConfig::startProgram(program &p) {
    
    //handle error opening stdout and stderr in parent
    // int out = ProgramConfig::openLog(this->stdout_logfile);
    // if (out < 0) {
    //     std::cout << "Error: The directory named as part of the path " 
    //         << this->stdout_logfile << "does not exist in section '" << 
    //         this->name << "' (file: '" << "./myconfigfile.conf" << "')\n";
    //     return errno;
    // }
    // int err = ProgramConfig::openLog(this->stderr_logfile);
    // if (err < 0) {
    //     std::cout << "Error: The directory named as part of the path " 
    //         << this->stdout_logfile << "does not exist in section '" << 
    //         this->name << "' (file: '" << "./myconfigfile.conf" << "')\n";
    //     close(out);
    //     return errno;
    // }

    //Fill the env variables into envptr that point to a char* 
    this->fillEnvp();

    //fork
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        p.state = State::Fatal;
        return;
    }
    if (pid == 0) {
        setpgid(0, 0); // L'enfant se fou dans son groupe 0
        // Execve takes in char* and not const char*, so we have to do this.
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char*)this->cmd.data(), nullptr};
        char ** envp = this->getEnvp();

        //redirecting to files
        // this->redirectFiles(out, err);

        execve("/bin/sh", argv, envp);

        perror("execve failed");
        _exit(127); // use _exit, not exit, in a failed post-fork child
    } else {
        // close(out);
        // close(err);
        setpgid(pid, pid); // Le parent fout l'enfant dans le groupe de son PID
                           // On fait les deux pour une histoire de race condition.

        // Were updating the program object passed in
        // We do this so that when we restart a program, we use the same object.
        p.pid = pid;
        p.state = State::Starting;
        p.start_time = time(nullptr);
        p.killing = false;
    }
}

bool ProgramConfig::requestStop(program &p, int client_fd) {
    if (p.state == State::Backoff) {
        p.state = State::Stopped;
        return true;
    }
    if (p.state != State::Running && p.state != State::Starting)
        return false;
    p.killing = true;
    p.state = State::Stopping;
    p.kill_deadline = time(nullptr) + stoptime;
    p.waiting_client = client_fd;
    kill(-p.pid, stopsignal); // group, since your cmds nest shells
    return true;
}

void ProgramConfig::tick(program &p, time_t now) {
    switch (p.state) {
    case State::Starting:
        if (now - p.start_time >= startsecs) { // survived long enough
            p.state = State::Running;
            p.curr_retries = 0;
        }
        break;
    case State::Backoff:
        if (now >= p.backoff_until)
            startProgram(p); // retry
        break;
    case State::Stopping:
        if (now > p.kill_deadline)
            kill(-p.pid, SIGKILL); // replaces killTimedOutPrograms
        break;
    default:
        break;
    }
}

void ProgramConfig::onExit(program &p, int status) {
    std::cout << "Program with PID" << p.pid << "Just ended" << std::endl;
    // we Asked for it
    if (p.killing) {
        p.state = State::Stopped;
        p.killing = false;
        if (p.restarting) { 
            p.restarting = false;
            startProgram(p); 
        }
        return;
    }
    time_t now = time(nullptr);
    bool diedTooEarly = p.state == State::Starting && now - p.start_time < startsecs;
    if (diedTooEarly) {
        // died too early: failed start
        if (++p.curr_retries > startretries) {
            p.state = State::Fatal;
        } else {
            p.state = State::Backoff;
            int delay = p.curr_retries * 2;
            p.backoff_until = now + delay;
        }
        return;
    }
    p.state = State::Exited;
    bool returnCodeIsInList = std::find(exitcodes.begin(), exitcodes.end(), WEXITSTATUS(status)) != exitcodes.end();
    bool diedNormally = WIFEXITED(status) && returnCodeIsInList;
    if (autorestart == AutoRestart::Always || (autorestart == AutoRestart::Unexpected && !diedNormally))
        startProgram(p);
}