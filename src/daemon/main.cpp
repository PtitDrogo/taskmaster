#include "ServerConfig.hpp"
#include "handlers.hpp"
#include "server.hpp"
#include "signal.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstring>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Error: argument expected" << std::endl;
    }
    Configs configs;

    configs.setConfigPath(argv[1]);

    int result = ini_parse(argv[1], handler, &configs);
    if (result < 0) {
        std::cerr << "Could not open config file\n";
        return 1;
    } else if (result > 0) {
        std::cerr << "Parse error on line " << result << "\n";
        return 1;
    }
    Logger::get().writePid(configs.server.getPidFile());
    if (Logger::get().open(configs.server.getLogFile()) < 0 &&
        Logger::get().writePid(configs.server.getPidFile()) < 0) {
        return errno;
    }
    Logger::get().info("Tasmasterd started with pid " + std::to_string(getpid()));
    std::cout << "I am the Daemon/Server !" << std::endl;
    openlog("taskmaster", LOG_PID | LOG_CONS, LOG_DAEMON);
    notifyDiscord("Taskmaster Started");

    int server_fd = ServerConfig::startDaemonServer();
    if (server_fd == -1)
        return EXIT_FAILURE;

    if (dropPrivilegesServer(configs.server.getUser()) == -1) {
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Supervisor started");
    syslog(LOG_WARNING, "This is a warning message");
    syslog(LOG_ERR, "This is an error message");

    setup_signal_handlers();

    std::cout << "Server listening on " << SOCK_PATH << std::endl;

    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    configs.startAllConfigs();

    while (true) {
        std::vector<pollfd> pfds = fds;
        std::vector<AttachedClients> attachedClients;
        configs.addAttachedFds(pfds, attachedClients);

        int ready = poll(pfds.data(), pfds.size(), 1000);
        if (ready < 0) {
            if (errno != EINTR) {
                perror("poll");
                break;
            }
            ready = 0;
        }

        if (ready > 0) {
            readAttachedClients(fds, pfds, attachedClients);
        }

        if (stop_requested)
            break;

        if (reload_requested) {
            reload_requested = 0;
            handleReload(-1, configs);
        }

        if (child_exited) {
            child_exited = 0;
            handleDeadProcesses(configs);
        }

        time_t now = time(nullptr);
        for (auto &[name, cfg] : configs.programs)
            for (auto &p : cfg.programs)
                cfg.tick(p, now);
        for (auto &[name, cfg] : configs.programs)
            cfg.reapExiled();

        configs.reapExiledConfigs();

        if (ready == 0)
            continue;

        // new client
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd >= 0) {
                fds.push_back({client_fd, POLLIN, 0});
                std::cout << "Client connected (fd=" << client_fd << ")\n";
            }
        }
        readClientInputs(fds, configs);
    }

    cleanup(fds, configs);
    return 0;
}
