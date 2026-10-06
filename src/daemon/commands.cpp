#include "server.hpp"
#include "utils.hpp"
#include <sstream>

int handleShutdown(int client_fd) {
    reply(client_fd, "Really shut the remote supervisord process down y/N?\n");

    char buf[256] = {0};
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);

    if (n <= 0) {
        return CLIENT_DISCONNECT;
    }
    std::string promptRes(buf);
    if (promptRes == "y") {
        reply(client_fd, "Shutting down\n");
        return SHUTDOWN;
    } else {
        reply(client_fd, "Shutdown aborted\n");
        return ABORTED;
    }
}

static ProgramConfig *findConfig(Configs &configs, const std::string &name) {
    auto it = configs.programs.find(name);
    return it == configs.programs.end() ? nullptr : &it->second;
}

static void handleStart(int client_fd, Configs &configs, const std::string &name) {
    ProgramConfig *cfg = findConfig(configs, name);
    if (!cfg) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    // autostart=false: the entries were never created
    if (cfg->programs.empty()) {
        cfg->startAllPrograms();
        reply(client_fd, name + ": started\n");
        return;
    }

    int started = 0;
    for (auto &p : cfg->programs) {
        if (p.state == State::Stopped || p.state == State::Exited || p.state == State::Fatal) {
            p.curr_retries = 0; // manual start = fresh retry budget
            cfg->startProgram(p);
            ++started;
        }
    }
    reply(client_fd, started ? name + ": started\n" : name + ": ERROR already started\n");
}

static void handleStop(int client_fd, Configs &configs, const std::string &name) {
    ProgramConfig *cfg = findConfig(configs, name);
    if (!cfg) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    int accepted = 0;
    int waiting = 0;
    for (auto &p : cfg->programs) {
        if (!cfg->requestStop(p, client_fd)) // false = not running
            continue;
        ++accepted;
        if (p.state == State::Stopping) // signal sent, answer comes later
            ++waiting;
    }

    if (accepted == 0)
        reply(client_fd, name + ": ERROR not running\n");
    else if (waiting == 0)
        reply(client_fd, name + ": stopped\n");
    // otherwise the main loop writes "stopped" when each process is reaped
}

static void handleRestart(int client_fd, Configs &configs, const std::string &name) {
    ProgramConfig *cfg = findConfig(configs, name);
    if (!cfg) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    // autostart=false: the entries were never created
    if (cfg->programs.empty()) {
        cfg->startAllPrograms();
        reply(client_fd, name + ": restarted\n");
        return;
    }

    bool should_wait = false;
    for (auto &p : cfg->programs) {
        if (p.state == State::Running || p.state == State::Starting) {
            cfg->requestStop(p, client_fd); // -> Stopping, SIGKILL after stoptime
            p.restarting = true;            // the main loop starts it once it is reaped
            should_wait = true;
        } else if (p.state != State::Stopping) { // Stopped, Exited, Fatal, Backoff
            p.curr_retries = 0;
            cfg->startProgram(p); // nothing to wait for, start right away
        }
    }
    if (!should_wait)
        reply(client_fd, name + ": restarted\n");
    // otherwise the main loop answers when the last process has been restarted
}

static const char *stateToString(State s) {
    switch (s) {
    case State::Stopped:
        return "STOPPED";
    case State::Starting:
        return "STARTING";
    case State::Running:
        return "RUNNING";
    case State::Backoff:
        return "BACKOFF";
    case State::Stopping:
        return "STOPPING";
    case State::Exited:
        return "EXITED";
    case State::Fatal:
        return "FATAL";
    }
    return "UNKNOWN";
}

void handleStatusCmd(int client_fd, const Configs &configs) {
    std::string out;
    time_t now = time(nullptr);

    for (const auto &[name, cfg] : configs.programs) {
        int i = 0;
        for (const auto &p : cfg.programs) {
            out += name + ":" + std::to_string(i++) + "  " + stateToString(p.state);
            if (p.state == State::Running || p.state == State::Starting || p.state == State::Stopping) {
                out += "  pid " + std::to_string(p.pid);
                out += ", uptime " + std::to_string(now - p.start_time) + "s";
            } else if (p.state == State::Backoff) {
                out += "  retry " + std::to_string(p.curr_retries);
            }
            out += "\n";
        }
    }
    reply(client_fd, out);
}

int handleCommands(int client_fd, std::string fullCmd, Configs &configs) {
    std::cout << "Full Command is " << fullCmd << std::endl;

    std::istringstream iss(fullCmd);
    std::string cmd;
    std::string arg;
    iss >> cmd;
    iss >> arg;

    if (cmd == "status") {
        std::cout << "Properly received the Status request !\n" << std::endl;
        handleStatusCmd(client_fd, configs);
    } else if (cmd == "shutdown") {
        return handleShutdown(client_fd);
    } else if (cmd == "start") {
        if (arg.empty()) {
            reply(client_fd, "ERROR usage: " + cmd + " <program>\n");
            return 1;
        }
        handleStart(client_fd, configs, arg);
    } else if (cmd == "stop") {
        if (arg.empty()) {
            reply(client_fd, "ERROR usage: " + cmd + " <program>\n");
            return 1;
        }
        handleStop(client_fd, configs, arg);
    } else if (cmd == "restart") {
        if (arg.empty()) {
            reply(client_fd, "ERROR usage: " + cmd + " <program>\n");
            return 1;
        }
        handleRestart(client_fd, configs, arg);
    } else {
        reply(client_fd, "ERROR unknown command\n");
    }
    return 1;
}