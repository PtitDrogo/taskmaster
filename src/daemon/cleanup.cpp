#include "utils.hpp"
#include <server.hpp>

static bool anyStopping(Configs &configs) {
    for (auto &[name, cfg] : configs.programs)
        for (auto &p : cfg.programs)
            if (p.state == State::Stopping)
                return true;
    return false;
}

static void killAllPrograms(Configs &configs) {
    for (auto &[name, cfg] : configs.programs)
        for (auto &p : cfg.programs)
            cfg.requestStop(p, -1); // sends stopsignal; ignores programs that aren't running

    while (anyStopping(configs)) {
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            auto [cfg, p] = configs.findByPid(pid);
            if (p)
                cfg->onExit(*p, status); // killing=true -> Stopped, no autorestart
        }
        time_t now = time(nullptr);
        for (auto &[name, cfg] : configs.programs)
            for (auto &p : cfg.programs)
                cfg.tick(p, now); // SIGKILL after stoptime
        usleep(50000);
    }
}

void cleanup(std::vector<pollfd> &fds, Configs &configs) {
    for (auto &pfd : fds) {
        reply(pfd.fd, "Server is shutting down\n");
        close(pfd.fd);
    }
    unlink(SOCK_PATH);
    killAllPrograms(configs);
}