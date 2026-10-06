#include "server.hpp"
#include <sstream>

int handleShutdown(int client_fd) {
    std::string response = "Really shut the remote supervisord process down y/N?";
    write(client_fd, response.c_str(), response.size());

    char buf[256] = {0};
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);

    if (n <= 0) {
        return CLIENT_DISCONNECT;
    }
    std::string promptRes(buf);
    if (promptRes == "y") {
        return SHUTDOWN;
    } else {
        return 1;
    }
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
            if (p.state == State::Running || p.state == State::Starting ||
                p.state == State::Stopping) {
                out += "  pid " + std::to_string(p.pid);
                out += ", uptime " + std::to_string(now - p.start_time) + "s";
            } else if (p.state == State::Backoff) {
                out += "  retry " + std::to_string(p.currRetries);
            }
            out += "\n";
        }
    }
    write(client_fd, out.c_str(), out.size());
}

int handleCommands(int client_fd, std::string fullCmd, Configs &configs) {
    std::cout << "Full Command is " << fullCmd << std::endl;

    std::istringstream iss(fullCmd);
    std::string cmd;
    std::string arg;
    iss >> cmd;
    iss >> arg;

    if (cmd == "STATUS") {
        std::cout << "Properly received the Status request !\n" << std::endl;
        handleStatusCmd(client_fd, configs);
    } else if (cmd == "shutdown") {
        return handleShutdown(client_fd);
    } else if (cmd == "start") {

    } else if (cmd == "stop") {

    } else if (cmd == "restart") {

    }

    else {
        std::string response = "ERROR unknown command\n";
        write(client_fd, response.c_str(), response.size());
    }
    return 1;
}