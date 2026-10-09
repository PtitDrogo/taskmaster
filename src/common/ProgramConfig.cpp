#include "ProgramConfig.hpp"
#include <unordered_map>

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
        for (auto &v : env)
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

namespace {
std::string stripQuotes(const std::string &s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool isValidEnvName(const std::string &name) {
    if (name.empty())
        return false;
    for (char c : name)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    return true;
}
} // namespace

int ProgramConfig::addEnvironnement(const std::string &value) {
    std::size_t start = 0;

    while (start < value.size()) {
        const std::size_t eq = value.find('=', start);
        if (eq == std::string::npos)
            return 1;

        bool inQuotes = false;
        std::size_t end = eq + 1;
        while (end < value.size() && (inQuotes || value[end] != ',')) {
            if (value[end] == '"')
                inQuotes = !inQuotes;
            end++;
        }

        const std::string name = value.substr(start, eq - start);
        const std::string val = stripQuotes(value.substr(eq + 1, end - eq - 1));

        if (!isValidEnvName(name))
            return -1;

        this->envMap[name] = val;
        start = end + 1;
    }
    return 0;
}

void ProgramConfig::fillEnvp(int i) {
    this->env.clear();
    this->envptr.clear();

    this->env.reserve(this->envMap.size() + 1);
    for (const auto &pair : this->envMap)
        this->env.push_back(pair.first + "=" + pair.second);
    this->env.push_back("number=" + std::to_string(i));

    this->envptr.reserve(this->env.size() + 1);
    for (auto &s : this->env)
        this->envptr.push_back(s.data());

    this->envptr.push_back(nullptr);
}

char **ProgramConfig::getEnvp() { return this->envptr.data(); }

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

void ProgramConfig::redirectFiles() {
    int devnull = open("/dev/null", O_RDONLY);
    if (dup2(devnull, STDIN_FILENO) < 0) {
        perror("dup2");
        exit(1);
    }
    close(devnull);

    if (dup2(fdout, STDOUT_FILENO) < 0) {
        perror("dup2");
        exit(1);
    }
    if (dup2(fderr, STDERR_FILENO) < 0) {
        perror("dup2");
        exit(1);
    }

    close(this->fdout);
    close(this->fderr);
}

int ProgramConfig::openRedirection() {
    int out = openLog(this->stdout_logfile, "stdout");
    if (out < 0) {
        int e = errno; // save before any other call
        std::cerr << "Error: cannot open '" << this->stdout_logfile << "' in section '" << this->name
                  << "': " << strerror(e) << "\n";
        return e;
    }

    int err = openLog(this->stderr_logfile, "stderr");
    if (err < 0) {
        int e = errno;
        std::cerr << "Error: cannot open '" << this->stderr_logfile << "' in section '" << this->name
                  << "': " << strerror(e) << "\n";
        close(out);
        return e;
    }

    this->fdout = out;
    this->fderr = err;
    return 0;
}

//restart program dont change
//reload and stop program create new files

void ProgramConfig::removeOlderLogFile(const std::string &identifier) {
    const fs::path directory = "/tmp";
    const std::string path = this->name + "-" + identifier + "---taskmaster";

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        const std::string name = entry.path().filename().string();

        if (name.find(path) != std::string::npos){
            std::error_code ec;
            fs::remove(entry.path(), ec);
        }
    }
}

int ProgramConfig::generateRandomFile(const std::string &identifier) {
    
    //Remove the older random file
    this->removeOlderLogFile(identifier);
    //create random file
    std::string path = "/tmp/" + this->name + "-" + identifier + "---taskmaster-XXXXXX.log";
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');
    
    // std::cout << "the path is " << buf.data() << std::endl;
    //create fd link to this file
    int fd = mkstemps(buf.data(), 4);
    if (fd < 0)
        return -1;

    if (identifier == "stdout")
        this->stdout_logfile = buf.data();
    else
        this->stderr_logfile = buf.data();
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}

// deux choses a regler
// supprimer le fichier dans le repertoire si il en existe un du meme type
// Rajouter le singleton pour avoir le fd de log

int ProgramConfig::openLog(const std::string &path, const std::string &identifier) {
    if (path == "AUTO" || path.empty())
        return this->generateRandomFile(identifier);
    const char *p = path == "NONE" ? "/dev/null" : path.c_str();
    if(identifier == "stdout")
        this->stdout_logfile = p;
    else
        this->stderr_logfile = p;
    if(std::string(p) != "/dev/null")
        this->removeOlderLogFile(identifier);
    return open(p, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
}

int ProgramConfig::startAllPrograms() {
    this->openRedirection();
    for (int i = 0; i < numprocs; ++i) {
        programs.emplace_back();
        this->startProgram(programs.back(), i);
    }
    return 1;
}

// we gotta just call /bin/sh on everything
void ProgramConfig::startProgram(program &p, int i) {
    // Fill the env variables into envptr that point to a char*
    this->fillEnvp(i);

    // fork
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        p.state = State::Fatal;
        return;
    }
    if (pid == 0) {
        setpgid(0, 0); // L'enfant se fou dans son groupe 0
        // Execve takes in char* and not const char*, so we have to do this.
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char *)this->cmd.data(), nullptr};
        char **envp = this->getEnvp();

        // redirecting to files
        this->redirectFiles();

        execve("/bin/sh", argv, envp);

        perror("execve failed");
        _exit(127); // use _exit, not exit, in a failed post-fork child
    } else {
        // close(this->fdout);
        // close(this->fderr);
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
            startProgram(p, 0); // retry
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
            startProgram(p, 0); 
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
        startProgram(p, 0);
}

// This could be done automatically with c++ 20 and better class organization, oh well !
// If we add something to ProgramConfig we have to add it here, boooooo
bool ProgramConfig::areSettingsEqual(const ProgramConfig &a, const ProgramConfig &b) {
    return std::tie(a.name, a.cmd, a.numprocs, a.autostart, a.startsecs, a.autorestart, a.exitcodes, a.startretries,
                    a.stopsignal, a.stoptime, a.discard_stdout, a.discard_stderr, a.stdout_logfile, a.stderr_logfile,
                    a.env, a.workingdir, a.umask) ==
           std::tie(b.name, b.cmd, b.numprocs, b.autostart, b.startsecs, b.autorestart, b.exitcodes, b.startretries,
                    b.stopsignal, b.stoptime, b.discard_stdout, b.discard_stderr, b.stdout_logfile, b.stderr_logfile,
                    b.env, b.workingdir, b.umask);
}

void ProgramConfig::copySettings(const ProgramConfig &src, ProgramConfig &dest) {
    std::tie(dest.name, dest.cmd, dest.numprocs, dest.autostart, dest.startsecs, dest.autorestart, dest.exitcodes,
             dest.startretries, dest.stopsignal, dest.stoptime, dest.discard_stdout, dest.discard_stderr,
             dest.stdout_logfile, dest.stderr_logfile, dest.env, dest.workingdir, dest.umask) =
        std::tie(src.name, src.cmd, src.numprocs, src.autostart, src.startsecs, src.autorestart, src.exitcodes,
                 src.startretries, src.stopsignal, src.stoptime, src.discard_stdout, src.discard_stderr,
                 src.stdout_logfile, src.stderr_logfile, src.env, src.workingdir, src.umask);
}