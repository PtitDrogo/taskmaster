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
              << "  starttime: " << starttime << "\n"
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

int ProgramConfig::parseSignals(std::string signal){
    const std::unordered_map<std::string, int> signals = {
        {"HUP", SIGHUP},   {"INT", SIGINT},   {"QUIT", SIGQUIT},
        {"KILL", SIGKILL}, {"TERM", SIGTERM}, {"USR1", SIGUSR1},
        {"USR2", SIGUSR2}
    };

    auto it = signals.find(signal);
    if (it == signals.end())
        return -1;
    return it->second;
}

static std::string stripQuotes(const std::string &s){
    if(s.size() >= 2 &&
       ((s.front() == '"' && s.back() == '"') ||
        (s.front() == '\'' && s.back() == '\''))){
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static bool isValidEnvName(const std::string &name){
    if(name.empty())
        return false;
    for(char c : name)
        if(!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    return true;
}

int ProgramConfig::addEnvironnement(std::string value){
    std::string name = "";
    std::string val = "";
    
    while(true){
        const std::size_t pos = value.find("=");
        if(pos == std::string::npos)
            break;
        name = value.substr(0, pos);

        const std::size_t pos1 = value.find_first_of(",");
        if(pos > pos1){
            return 1;
        }
        if(pos1 != std::string::npos){
            val = value.substr(pos + 1, pos1 - pos - 1);
            val = stripQuotes(val);
            if(!isValidEnvName(val))
                return -1;
            this->env.push_back(name + "=" + val);
            value = value.substr(pos1 + 1, value.length() - 1 - pos1);
        }
        else{
            val = value.substr(pos + 1, value.length() - pos);
            val = stripQuotes(val);
            if(!isValidEnvName(val))
                return -1;
            this->env.push_back(name + "=" + val);
            return 0;
        }
    }
    return 1;
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
        }else if (setting == "stopsignal"){
            int signal = parseSignals(value);
            if(signal != -1){
                stopsignal = signal;
            }
        }else if (setting == "environment"){
            int err = addEnvironnement(value);
            if(err){
                std::cerr << "Unknown setting: " << setting << " in [" << name << "]\n";
                return 0;
            }
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

void ProgramConfig::redirectFiles(int fd_out, int fd_err){
    int devnull = open("/dev/null", O_RDONLY);
    dup2(devnull, STDIN_FILENO);
    close(devnull);

    dup2(fd_out, STDOUT_FILENO);
    close (fd_out);
    dup2(fd_err, STDERR_FILENO);
    close (fd_err);
}

static int openLog(const std::string &path) {
    const char *p = path.empty() ? "/dev/null" : path.c_str();
    return open(p, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
}

// char *envp[] ProgramConfig::fillEnvp(){
//     auto it = this->env.begin();
    
// }

// we gotta just call /bin/sh on everything
int ProgramConfig::createProgram() {

    //handle error opening stdout and stderr in parent
    int out = openLog(this->stdout_logfile);
    if (out < 0) {
        std::cout << "Error: The directory named as part of the path " 
            << this->stdout_logfile << "does not exist in section '" << 
            this->name << "' (file: '" << "./myconfigfile.conf" << "')\n";
        return errno;
    }
    int err = openLog(this->stderr_logfile);
    if (err < 0) {
        std::cout << "Error: The directory named as part of the path " 
            << this->stdout_logfile << "does not exist in section '" << 
            this->name << "' (file: '" << "./myconfigfile.conf" << "')\n";
        close(out);
        return errno;
    }

    //fork
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return errno;
    } else if (pid == 0) {
        // Execve takes in char* and not const char*, so we have to do this.
        char *argv[] = {(char *)"/bin/sh", (char *)"-c", (char*)this->cmd.data(), nullptr};
        char *envp[] = {nullptr}; // Pass env later.

        //redirecting to files
        this->redirectFiles(out, err);

        execve("/bin/sh", argv, envp);

        // Only reached if execve fails
        perror("execve failed");
        _exit(127); // use _exit, not exit, in a failed post-fork child
    } else {
        close(out);
        close(err);
        // Adding the pid of this particular instance to the list to be waited on later.
        programs.push_back({pid, "Test starting state"});

        // This waiting thing happens later or smth idk.
        //  int status;
        //  waitpid(pid, &status, 0);
        //  if (WIFEXITED(status)) {
        //      std::cout << "Child exited with " << WEXITSTATUS(status) << "\n";
        //  }
    }
    return 0;
}