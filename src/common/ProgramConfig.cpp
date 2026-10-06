#include "ProgramConfig.hpp"

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
              << "  workingdir: " << workingdir << "\n"
              << "  umask: " << std::oct << umask << std::dec << "\n";

    if (!env.empty()) {
        std::cout << "  env:\n";
        for (auto &[k, v] : env)
            std::cout << k << "=" << v << "\n";
    }
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
        }

        else {
            std::cerr << "Unknown setting: " << setting << " in [" << name << "]\n";
            return 0;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 0;
    }

    return 1;
}

int ProgramConfig::startAllPrograms() {
    for (int i = 0; i < numprocs; ++i) {
        programs.emplace_back();
        startProgram(programs.back());
    }
    return 1;
}

// we gotta just call /bin/sh on everything
void ProgramConfig::startProgram(program &p) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        p.state = State::Fatal;
        return;
    }
    if (pid == 0) {
        setpgid(0, 0); // L'enfant se fou dans son groupe 0
        // Execve takes in char* and not const char*, so we have to do this.
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char *)cmd.c_str(), nullptr};
        char *envp[] = {nullptr}; // Pass env later.
        execve("/bin/sh", argv, envp);

        perror("execve failed");
        _exit(127); // use _exit, not exit, in a failed post-fork child
    } else {
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
            p.currRetries = 0;
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
    // we Asked for it
    if (p.killing) {
        p.state = State::Stopped;
        p.killing = false;
        return;
    }
    time_t now = time(nullptr);
    bool diedTooEarly = p.state == State::Starting && now - p.start_time < startsecs;
    if (diedTooEarly) {
        // died too early: failed start
        if (++p.currRetries > startretries) {
            p.state = State::Fatal;
        } else {
            p.state = State::Backoff;
            int delay = p.currRetries * 2;
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