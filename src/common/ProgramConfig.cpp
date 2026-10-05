#include "ProgramConfig.hpp"

ProgramConfig::ProgramConfig(/* args */) {}

ProgramConfig::~ProgramConfig() {}

void ProgramConfig::printSettings() const {
    std::cout << "Section: " << name << "\n"
              << "  command: " << cmd << "\n"
              << "  autostart: " << std::boolalpha << autostart << "\n"
              << "  numprocs: " << numprocs << "\n"
              << "  autorestart: " << static_cast<int>(autorestart) << "\n"
              << "  starttime: " << starttime << "\n"
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
        } else if (setting == "starttime") {
            starttime = std::stoi(value);
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
    for (int i = 0; i < numprocs; ++i)
        if (startProgram() == -1)
            return -1;
    return 1;
}

// we gotta just call /bin/sh on everything
pid_t ProgramConfig::startProgram() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        setpgid(0, 0); // L'enfant se fou dans son groupe 0
        // Execve takes in char* and not const char*, so we have to do this.
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char *)cmd.c_str(), nullptr};
        char *envp[] = {nullptr}; // Pass env later.
        execve("/bin/sh", argv, envp);

        // Only reached if execve fails
        perror("execve failed");
        _exit(127); // use _exit, not exit, in a failed post-fork child
    } else {
        setpgid(pid, pid); // Le parent fou l'enfant dans le groupe de son PID
        // On fait les deux pour une histoire de race condition.

        // Adding the pid of this particular instance to the list to be waited on later.
        programs.push_back({pid, "Test starting state"});

        // This waiting thing happens later or smth idk.
        //  int status;
        //  waitpid(pid, &status, 0);
        //  if (WIFEXITED(status)) {
        //      std::cout << "Child exited with " << WEXITSTATUS(status) << "\n";
        //  }
    }
    return pid;
}