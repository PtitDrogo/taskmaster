#include "server.hpp"

#include <cstring>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#define SOCK_PATH "/tmp/supervisor.sock"

struct Configs {
    ServerConfig server;
    std::map<std::string, ProgramConfig> programs;

    void printSettings() const {
        server.printSettings();
        for (auto &[section, cfg] : programs) {
            std::cout << "Section: " << section << "\n";
            cfg.printSettings();
        }
    }
};

/*
Exemple:
[program:web]
command = python app.py
autostart = true
numprocs = 3

Section = [program:web] //Same for all 3 key/value pair
name = "command"
value = "python app.py"

*/

static int handler(void *user, const char *section, const char *name, const char *value) {
    auto *cfg = static_cast<Configs *>(user);
    std::string sect(section), setting(name), val(value);
    int err = 1;
    if (sect.rfind("program:", 0) == 0) {
        std::string progname = sect.substr(8); // strip "program:"
        ProgramConfig &pc = cfg->programs[progname];
        err = pc.parseSetting(setting, val);
    } else if (sect == "unix_http_server" || sect == "inet_http_server" || sect == "supervisord") {
        err = cfg->server.parseSetting(setting, val);
    } else if (sect.rfind("rpcinterface:", 0) == 0) {
        // Idk what that is I dont think we need to handle that
    }
    return err;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Error: argument expected" << std::endl;
    }
    Configs configs;

    int result = ini_parse(argv[1], handler, &configs);
    if (result < 0) {
        std::cerr << "Could not open config file\n";
        return 1;
    } else if (result > 0) {
        std::cerr << "Parse error on line " << result << "\n";
        return 1;
    }

    std::cout << "I am the Daemon/Server !" << std::endl;
    configs.printSettings();

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    unlink(SOCK_PATH); // remove old socket file if it exists

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Server listening on " << SOCK_PATH << std::endl;

    bool flag = true;

    // pollfd list: index 0 is always the listening socket, rest are clients
    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    if (dropPrivileges("tfreydie") == -1 && dropPrivileges("bpoyet") == -1) {
        std::cerr << "Error trying to default to tfreydie or bpoyet user privileges\n";
        return 1;
        // Dont hardcode this before sending it :)
    }

    while (true) {
        int ready = poll(fds.data(), fds.size(), 1000);
        if (ready < 0) {
            if (errno == EINTR)
                continue; // interrupted by a signal, just retry
            perror("poll");
            break;
        }

        if (ready == 0) {
            // Check on processes, nothing happened
            continue;
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd >= 0) {
                fds.push_back({client_fd, POLLIN, 0});
                std::cout << "Client connected (fd=" << client_fd << ")\n";
            }
        }

        // backward so we can just safely remove
        for (size_t i = fds.size(); i-- > 1;) {
            if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
                continue;

            int client_fd = fds[i].fd;
            char buf[256];
            ssize_t n = read(client_fd, buf, sizeof(buf) - 1);

            if (n <= 0) {
                std::cout << "Client disconnected (fd=" << client_fd << ")\n";
                close(client_fd);
                fds.erase(fds.begin() + i);
                continue;
            }

            buf[n] = '\0';
            std::string cmd(buf);
            std::string response;
            std::cout << "Command is " << cmd << std::endl;
            if (cmd == "SET true") {
                flag = true;
                response = "OK flag set to true\n";
            } else if (cmd == "SET false") {
                flag = false;
                response = "OK flag set to false\n";
            } else if (cmd == "GET") {
                response = std::string("STATUS ") + (flag ? "true" : "false") + "\n";
            } else {
                response = "ERROR unknown command\n";
            }

            write(client_fd, response.c_str(), response.size());
        }
    }

    for (auto &pfd : fds)
        close(pfd.fd);
    unlink(SOCK_PATH);

    return 0;
}
