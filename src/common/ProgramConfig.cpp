#include "ProgramConfig.hpp"
#include "utils.hpp"
#include <pty.h>
#include <sys/stat.h>
#include "ServerConfig.hpp"
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
              << "  stopwaitsecs: " << stopwaitsecs << "\n"
              << "  stdout logfile: " << stdout_logfile << "\n"
              << "  stderr logfile: " << stderr_logfile << "\n"
              << "  workingdir: " << workingdir << "\n"
              << "  umask: " << std::oct << m_umask << std::dec << "\n";

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

void ProgramConfig::fillEnvp() {
    this->env.clear();
    this->envptr.clear();

    this->env.reserve(this->envMap.size());
    for (const auto &pair : this->envMap)
        this->env.push_back(pair.first + "=" + pair.second);

    this->envptr.reserve(this->env.size() + 1);
    for (auto &s : this->env)
        this->envptr.push_back(s.data());

    this->envptr.push_back(nullptr);
}

char **ProgramConfig::getEnvp() { return this->envptr.data(); }

int ProgramConfig::getStartSecs() const{
    return this->startsecs;
}

int ProgramConfig::parseSetting(const std::string &setting, const std::string &value) {
    try {
        if (setting == "command") {
            cmd = value;
        } else if (setting == "autostart") {
            autostart = (value == "true");
        } else if (setting == "user") {
            user = value;
        } else if (setting == "numprocs") {
            numprocs = std::stoi(value);
        } else if (setting == "startsecs") {
            startsecs = std::stoi(value);
            if (startsecs <= 0) {
                throw std::out_of_range("startsecs must be a value above 0");
            }
        } else if (setting == "startretries") {
            startretries = std::stoi(value);
        } else if (setting == "stopwaitsecs") {
            stopwaitsecs = std::stoi(value);
        } else if (setting == "directory") {
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
            m_umask = std::stoi(value, nullptr, 8);
        } else if (setting == "autorestart") {
            if (value == "always")
                autorestart = AutoRestart::Always;
            else if (value == "never")
                autorestart = AutoRestart::Never;
            else if (value == "unexpected")
                autorestart = AutoRestart::Unexpected;
        } else if (setting == "exitcodes") {
            exitcodes.push_back(std::stoi(value));
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

    if (dup2(this->fdout, STDOUT_FILENO) < 0) {
        perror("dup2");
        exit(1);
    }
    if (dup2(this->fderr, STDERR_FILENO) < 0) {
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

void ProgramConfig::removeOlderLogFile(const std::string &identifier) {
    const fs::path directory = "/tmp";
    const std::string path = this->name + "-" + identifier + "---taskmaster";

    for (const auto &entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file())
            continue;

        const std::string name = entry.path().filename().string();

        if (name.find(path) != std::string::npos) {
            std::error_code ec;
            fs::remove(entry.path(), ec);
        }
    }
}

int ProgramConfig::generateRandomFile(const std::string &identifier) {

    // Remove the older random file
    this->removeOlderLogFile(identifier);
    // create random file
    std::string path = "/tmp/" + this->name + "-" + identifier + "---taskmaster-XXXXXX.log";
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');

    // std::cout << "the path is " << buf.data() << std::endl;
    // create fd link to this file
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


int ProgramConfig::openLog(const std::string &path, const std::string &identifier) {
    if (path == "AUTO" || path.empty())
        return this->generateRandomFile(identifier);
    const char *p = path == "NONE" ? "/dev/null" : path.c_str();
    if (identifier == "stdout")
        this->stdout_logfile = p;
    else
        this->stderr_logfile = p;
    if (std::string(p) != "/dev/null")
        this->removeOlderLogFile(identifier);
    return open(p, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
}

int ProgramConfig::startAllPrograms() {
    this->openRedirection();
    for (int i = 0; i < numprocs; ++i) {
        this->startProgram(programs.emplace_back());
    }
    return 1;
}

void ProgramConfig::scaleToNumprocs() {
    int active = 0;
    for (auto &p : programs)
        if (!p.exiled)
            ++active;

    while (active < numprocs) {
        startProgram(programs.emplace_back());
        ++active;
    }

    for (size_t i = programs.size(); i-- > 0 && active > numprocs;) {
        if (programs[i].exiled)
            continue;
        programs[i].exiled = true;
        programs[i].restarting = false; // handleModifiedPrograms set it, cancel it
        requestStop(programs[i], -1);   // no-op if already Stopping
        --active;
    }
}

void ProgramConfig::reapExiled() {
    for (auto it = programs.begin(); it != programs.end();) {
        bool dead = it->state == State::Stopped || it->state == State::Exited || it->state == State::Fatal;
        if (it->exiled && dead) {
            closeMaster(*it);
            closeErr(*it);
            it = programs.erase(it);
        } else {
            ++it;
        }
    }
}

bool ProgramConfig::isNameTaken(std::string name) {
    for (auto &p : this->programs) {
        if (p.name == name) {
            return true;
        }
    };
    return false;
}

void ProgramConfig::setProgramName(program &p) {
    if (!p.name.empty())
        return;
    size_t len = this->programs.size();
    std::string add_on = len == 1 ? "" : "_" + std::to_string(len);
    std::string name = this->name + add_on;
    int backup = 1;
    while (isNameTaken(name)) {
        name = this->name + add_on + "_" + std::to_string(++backup);
    }
    p.name = name;
    return;
}

// This always starts a program
// Logic for not starting a program should be outside of this.
void ProgramConfig::startProgram(program &p) {
    setProgramName(p);
    this->fillEnvp();
    this->closeMaster(p);
    this->closeErr(p);

    int errp[2];
    if (pipe2(errp, O_CLOEXEC) == -1) {
        perror("pipe2");
        p.state = State::Fatal;
        return;
    }

    pid_t pid = forkpty(&p.master_fd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        perror("forkpty");
        close(errp[0]);
        close(errp[1]);
        p.state = State::Fatal;
        return;
    }
    if (pid == 0) {
        dup2(errp[1], STDERR_FILENO); // fd 2 now goes to the pipe, not the pty
        if (dropPrivilegesProgram(this->user) == -1) {
            _exit(127);
        }
        if (!this->workingdir.empty() && chdir(this->workingdir.c_str()) == -1) {
            perror("chdir failed");
            _exit(127);
        }
        ::umask(m_umask);
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char *)cmd.data(), nullptr};
        execve("/bin/sh", argv, getEnvp());
        _exit(127);
    }
    Logger::get().info("spawned: '" + this->name + "' with pid " + std::to_string(pid));
    close(errp[1]); // parent only keeps the read end
    p.err_fd = errp[0];
    fcntl(p.err_fd, F_SETFL, O_NONBLOCK);

    fcntl(p.master_fd, F_SETFD, FD_CLOEXEC);
    fcntl(p.master_fd, F_SETFL, O_NONBLOCK);
    p.pid = pid;
    p.state = State::Starting;
    p.start_time = time(nullptr);
    p.killing = false;
    
}

void ProgramConfig::closeMaster(program &p) {
    if (p.attached_client != -1) {
        reply(p.attached_client, std::string(1, DETACH_KEY)); // tells client to leave attach mode
        p.attached_client = -1;
    }
    if (p.master_fd != -1) {
        close(p.master_fd);
        p.master_fd = -1;
    }
}

void ProgramConfig::onMasterReadable(program &p) {
    char buf[4096];
    ssize_t n = read(p.master_fd, buf, sizeof buf);
    if (n > 0) {
        if (this->fdout > 0)
            write(this->fdout, buf, n); 
        p.scrollback.append(buf, n);
        if (p.scrollback.size() > SCROLLBACK_MAX)
            p.scrollback.erase(0, p.scrollback.size() - SCROLLBACK_MAX);
        if (p.attached_client != -1)
            write(p.attached_client, buf, n);
    } else if (n == 0 || (errno != EAGAIN && errno != EINTR)) { // EIO = slave closed
        closeMaster(p);
    }
}

void ProgramConfig::closeErr(program &p) {
    if (p.err_fd != -1) {
        close(p.err_fd);
        p.err_fd = -1;
    }
}

void ProgramConfig::onErrReadable(program &p) {
    char buf[4096];
    ssize_t n = read(p.err_fd, buf, sizeof buf);
    if (n > 0) {
        if (fderr > 0)
            write(fderr, buf, n); // stderr log
        p.scrollback.append(buf, n);
        if (p.scrollback.size() > SCROLLBACK_MAX)
            p.scrollback.erase(0, p.scrollback.size() - SCROLLBACK_MAX);
        if (p.attached_client != -1)
            write(p.attached_client, buf, n);
    } else if (n == 0 || (errno != EAGAIN && errno != EINTR)) {
        closeErr(p);
    }
}

bool ProgramConfig::requestStop(program &p, int client_fd) {
    notifyDiscord("Process PID: " + std::to_string(p.pid) + "is stopping");
    if (p.state == State::Backoff) {
        p.state = State::Stopped;
        std::cout << "je suis dans l'aute killing" << std::endl;
        return true;
    }
    if (p.state != State::Running && p.state != State::Starting)
        return false;
    p.killing = true;
    p.state = State::Stopping;
    p.kill_deadline = time(nullptr) + stopwaitsecs;
    p.waiting_client = client_fd;
    kill(-p.pid, stopsignal); // group, since your cmds nest shells
    return true;
}

void ProgramConfig::tick(program &p, time_t now) {
    switch (p.state) {
    case State::Starting:
        if (now - p.start_time >= startsecs) { // survived long enough
            // p.state = State::Running;
            this->setState(State::Running, p);
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
    p.exitcode = WEXITSTATUS(status);
    std::cout << "the exit code is " << p.exitcode << std::endl;
    std::cout << "Program with PID" << p.pid << "Just ended" << std::endl;

    // This should only happen due to a reload with less numprocs
    if (p.exiled) {
        p.state = State::Stopped;
        p.killing = false;
        p.restarting = false;
        return;
    }

    // we Asked for it
    if (p.killing) {
        // p.state = State::Stopped;
        bool stopstate = WIFSIGNALED(status);
        p.exitsig = stopstate ? WTERMSIG(status) : -1;
        this->setState(State::Stopped, p);
        p.killing = false;
        // We want it to restart, this could be because of the restart command
        // or because we reloaded and it has the autoStart Feature.
        if (p.restarting) {
            p.restarting = false;
            startProgram(p);
        }
        return;
    }
    time_t now = time(nullptr);
    bool diedTooEarly = p.state == State::Starting && now - p.start_time <= startsecs;
    if (diedTooEarly) {
        
        this->setState(State::Exited, p);
        // died too early: failed start
        if (++p.curr_retries > startretries) {
            // p.state = State::Fatal;
            std::cout << "je suis dans le fatal" << std::endl;
            this->setState(State::Fatal, p);
        } else {
            std::cout << "je suis dans le backoff" << std::endl;
            p.state = State::Backoff;
            int delay = p.curr_retries * 2;
            p.backoff_until = now + delay;
        }
        return;
    }
    // p.state = State::Exited;
    this->setState(State::Exited, p);
    bool returnCodeIsInList = std::find(exitcodes.begin(), exitcodes.end(), WEXITSTATUS(status)) != exitcodes.end();
    bool diedNormally = WIFEXITED(status) && returnCodeIsInList;
    if (autorestart == AutoRestart::Always || (autorestart == AutoRestart::Unexpected && !diedNormally))
        startProgram(p);
}

void ProgramConfig::setState(State state, program &p){
    p.state = state;
    Logger::get().printState(state, p, *this);
}

// This could be done automatically with c++ 20 and better class organization, oh well !
// If we add something to ProgramConfig we have to add it here, boooooo
bool ProgramConfig::areSettingsEqual(const ProgramConfig &a, const ProgramConfig &b) {
    return std::tie(a.name, a.cmd, a.numprocs, a.autostart, a.startsecs, a.autorestart, a.exitcodes, a.startretries,
                    a.stopsignal, a.stopwaitsecs, a.discard_stdout, a.discard_stderr, a.stdout_logfile,
                    a.stderr_logfile, a.envMap, a.workingdir, a.m_umask) ==
           std::tie(b.name, b.cmd, b.numprocs, b.autostart, b.startsecs, b.autorestart, b.exitcodes, b.startretries,
                    b.stopsignal, b.stopwaitsecs, b.discard_stdout, b.discard_stderr, b.stdout_logfile,
                    b.stderr_logfile, b.envMap, b.workingdir, b.m_umask);
}

void ProgramConfig::copySettings(const ProgramConfig &src, ProgramConfig &dest) {
    std::tie(dest.name, dest.cmd, dest.numprocs, dest.autostart, dest.startsecs, dest.autorestart, dest.exitcodes,
             dest.startretries, dest.stopsignal, dest.stopwaitsecs, dest.discard_stdout, dest.discard_stderr,
             dest.stdout_logfile, dest.stderr_logfile, dest.envMap, dest.workingdir, dest.m_umask) =
        std::tie(src.name, src.cmd, src.numprocs, src.autostart, src.startsecs, src.autorestart, src.exitcodes,
                 src.startretries, src.stopsignal, src.stopwaitsecs, src.discard_stdout, src.discard_stderr,
                 src.stdout_logfile, src.stderr_logfile, src.envMap, src.workingdir, src.m_umask);
}