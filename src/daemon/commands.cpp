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

void handleStatusCmd(int client_fd, const Configs &configs) {
    std::cout << "Properly received the Status request !\n" << std::endl;
    for (const auto &config : configs.programs) {
        for (const auto &program : config.second.programs) {
            if (kill(program.pid, 0) == 0) {
                std::string response = std::string("Program with PID ") + std::to_string(program.pid) +
                                       std::string("is good and well !\n");
                write(client_fd, response.c_str(), response.size());
                // process exists (you have permission to signal it)
            } else if (errno == ESRCH) {
                std::string response =
                    std::string("Program with PID ") + std::to_string(program.pid) + std::string("dead and buried !\n");
                write(client_fd, response.c_str(), response.size());
                // no such process — already dead and reaped, or never existed
            } else {
                std::string response = std::string("Program with PID ") + std::to_string(program.pid) +
                                       std::string("is in a state idk what it means !\n");
                write(client_fd, response.c_str(), response.size());
                // idk
            }
        }
    }
}

int handleCommands(int client_fd, std::string fullCmd, const Configs &configs) {
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