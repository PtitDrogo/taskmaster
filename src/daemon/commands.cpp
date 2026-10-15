#include "server.hpp"
#include "utils.hpp"
#include <sstream>

static int handleShutdown(int client_fd) {
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

static void handleStart(int client_fd, Configs &configs, const std::string &name) {
    auto [cfg, p] = configs.findByProgramName(name);
    if (!cfg || !p) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }
    // autostart=false: the entries were never created
    if (cfg->programs.empty()) {
        cfg->openRedirection();
        cfg->startProgram(*p);
        reply(client_fd, name + ": started\n");
        return;
    }

    int started = 0;
    bool stopped = p->state == State::Stopped || p->state == State::Exited || p->state == State::Fatal;

    if (!p->exiled && stopped) {
        p->curr_retries = 0; // manual start = fresh retry budget
        cfg->startProgram(*p);
        ++started;
    }
    reply(client_fd, started ? name + ": started\n" : name + ": ERROR already started\n");
}

static void handleStop(int client_fd, Configs &configs, const std::string &name) {
    auto [cfg, p] = configs.findByProgramName(name);
    if (!cfg || !p) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    int accepted = 0;
    int waiting = 0;

    if (cfg->requestStop(*p, client_fd))
        ++accepted;
    if (p->state == State::Stopping)
        ++waiting;

    if (accepted == 0)
        reply(client_fd, name + ": ERROR not running\n");
    else if (waiting == 0)
        reply(client_fd, name + ": stopped\n");
    // otherwise the main loop writes "stopped" when each process is reaped
}

static void handleRestart(int client_fd, Configs &configs, const std::string &name) {
    auto [cfg, p] = configs.findByProgramName(name);
    if (!cfg || !p) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    // autostart=false: the entries were never created
    if (cfg->programs.empty()) {
        cfg->openRedirection();
        cfg->startProgram(*p);
        reply(client_fd, name + ": started\n");
        return;
    }

    bool should_wait = false;
    if (p->exiled) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }
    if (p->state == State::Running || p->state == State::Starting) {
        cfg->requestStop(*p, client_fd); // -> Stopping, SIGKILL after stopwaitsecs
        p->restarting = true;            // the main loop starts it once it is reaped
        should_wait = true;
    } else if (p->state != State::Stopping) { // Stopped, Exited, Fatal, Backoff
        p->curr_retries = 0;
        cfg->startProgram(*p); // nothing to wait for, start right away
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

static void handleStatusCmd(int client_fd, const Configs &configs) {
    std::string out;
    time_t now = time(nullptr);

    for (const auto &[name, cfg] : configs.m_configs) {
        if (cfg.isExiled()) {
            continue;
        }
        for (const auto &p : cfg.programs) {
            if (p.exiled)
                continue;
            out += p.name + ":  " + stateToString(p.state);
            if (p.state == State::Running || p.state == State::Starting || p.state == State::Stopping) {
                out += "  pid " + std::to_string(p.pid);
                out += ", uptime " + std::to_string(now - p.start_time) + "s";
            } else if (p.state == State::Backoff) {
                out += "  retry " + std::to_string(p.curr_retries);
            }
            out += "\n";
        }
    }
    if (out.empty()) {
        out += "No processes running.\n";
    }
    reply(client_fd, out);
}

static void handleAttach(int client_fd, Configs &configs, const std::string &name) {
    auto [cfg, p] = configs.findByProgramName(name);
    if (!cfg || !p) {
        reply(client_fd, "ERROR no such program: " + name + "\n");
        return;
    }

    if (p->exiled) {
        reply(client_fd, "ERROR not running\n");
        return;
    }
    if (p->master_fd == -1 || (p->state != State::Running && p->state != State::Starting)) {
        reply(client_fd, "ERROR not running\n");
        return;
    }
    if (p->attached_client != -1) {
        reply(client_fd, "ERROR already attached\n");
        return;
    }

    reply(client_fd, "ATTACHED\n");
    reply(client_fd, p->scrollback); // replay recent output
    p->attached_client = client_fd;
}

int handleCommands(int client_fd, std::string fullCmd, Configs &configs) {
    std::cout << "Full Command is " << fullCmd << std::endl;

    std::istringstream iss(fullCmd);
    std::string cmd;
    std::string arg;
    iss >> cmd >> arg;

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
    } else if (cmd == "attach") {
        if (arg.empty()) {
            reply(client_fd, "ERROR usage: attach <program> [index]\n");
            return 1;
        }
        handleAttach(client_fd, configs, arg);
    } else if (cmd == "restart") {
        if (arg.empty()) {
            reply(client_fd, "ERROR usage: " + cmd + " <program>\n");
            return 1;
        }
        handleRestart(client_fd, configs, arg);
    } else if (cmd == "reload") {
        std::cout << "Properly received the Reload request !\n" << std::endl;
        handleReload(client_fd, configs);
    } else {
        reply(client_fd, "ERROR unknown command\n");
    }
    return 1;
}