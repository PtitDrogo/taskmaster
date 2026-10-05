#include "server.hpp"

void handleCommands(int client_fd, std::string cmd, const Configs &configs) {
    std::cout << "Command is " << cmd << std::endl;

    if (cmd == "STATUS") {
        std::cout << "Properly received the Status request !\n" << std::endl;
        handleStatusCmd(client_fd, configs);
    } else {
        std::string response = "ERROR unknown command\n";
        write(client_fd, response.c_str(), response.size());
    }
}

void handleStatusCmd(int client_fd, const Configs &configs) {
    std::cout << "Properly received the Status request !\n" << std::endl;
    for (auto config : configs.programs) {
        for (auto program : config.second.programs) {
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