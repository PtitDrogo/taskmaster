#include "ServerConfig.hpp"
#include "server.hpp"
#include <csignal>
#include <cstring>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

volatile sig_atomic_t child_exited = 0;

void sigchld_handler(int) {
    child_exited = 1; // just set a flag, do real work outside the handler
}

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

void cleanup(std::vector<pollfd> &fds, Configs &configs) {
    for (auto &pfd : fds)
        close(pfd.fd);
    unlink(SOCK_PATH);
    // Killing all child programs.
    for (auto &programMap : configs.programs) {
        for (auto &program : programMap.second.programs) {
            std::cout << "Killing the program" << program.pid << std::endl;
            kill(-program.pid, SIGTERM); // askip faudra ptet faire des trucs en plus.
        }
    }
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

    int server_fd = ServerConfig::startDaemonServer();
    if (server_fd == -1)
        return EXIT_FAILURE;

    // signal to know whats going on with children
    // When a child dies, the kernel sends SIGCHLD to its parent.
    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart whatever syscall the signal interrupted (Not guaranted)
    sigaction(SIGCHLD, &sa, nullptr);

    std::cout << "Server listening on " << SOCK_PATH << std::endl;

    // pollfd list: index 0 is always the listening socket, rest are clients
    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    if (dropPrivileges("tfreydie") == -1 && dropPrivileges("bpoyet") == -1) {
        std::cerr << "Error trying to default to tfreydie or bpoyet user privileges\n";
        return 1;
        // Dont hardcode this before sending it :)
    }

    // Launch All programs of all configs
    for (auto &[name, cfg] : configs.programs) {
        if (cfg.shouldAutostart())
            if (cfg.startAllPrograms() == -1) {
                cleanup(fds, configs);
                return EXIT_FAILURE;
            }
    }

    while (true) {
        int ready = poll(fds.data(), fds.size(), 1000);
        if (ready < 0) {
            if (errno != EINTR) {
                perror("poll");
                break;
            }
            ready = 0; // We got interrupted we still do down.
        }

        if (child_exited) {
            child_exited = 0;
            int status;
            pid_t pid;

            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                auto [cfg, p] = configs.findByPid(pid);
                if (!p)
                    continue;
                std::cout << "Program with PID" << pid << "Just ended" << std::endl;
                cfg->onExit(*p, status);

                if (p->state == State::Stopped && p->waiting_client != -1) {
                    write(p->waiting_client, "stopped\n", 8);
                    p->waiting_client = -1;
                }
            }
        }

        time_t now = time(nullptr);
        for (auto &[name, cfg] : configs.programs)
            for (auto &p : cfg.programs)
                cfg.tick(p, now);

        if (ready == 0)
            continue;

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
            char buf[256] = {0};
            ssize_t n = read(client_fd, buf, sizeof(buf) - 1);

            if (n <= 0) {
                std::cout << "Client disconnected (fd=" << client_fd << ")\n";
                close(client_fd);
                fds.erase(fds.begin() + i);
                continue;
            }

            std::string cmd(buf);
            int err = handleCommands(client_fd, cmd, configs);
            if (err == SHUTDOWN) {
                cleanup(fds, configs);
                return 0;
            } else if (err == CLIENT_DISCONNECT) {
                std::cout << "Client disconnected (fd=" << client_fd << ")\n";
                close(client_fd);
                fds.erase(fds.begin() + i);
            }
        }
    }
    cleanup(fds, configs);
    return 0;
}
