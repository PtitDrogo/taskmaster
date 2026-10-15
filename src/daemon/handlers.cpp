#include "handlers.hpp"
#include "utils.hpp"

int handler(void *user, const char *section, const char *name, const char *value) {
    auto *cfg = static_cast<Configs *>(user);
    std::string sect(section), setting(name), val(value);
    int err = 1;
    if (sect.rfind("program:", 0) == 0) {
        std::string progname = sect.substr(8);
        ProgramConfig &pc = cfg->programs[progname];
        pc.setName(progname);
        err = pc.parseSetting(setting, val);
    } else if (sect == "unix_http_server" || sect == "inet_http_server" || sect == "supervisord") {
        err = cfg->server.parseSetting(setting, val);
    }
    return err;
}

void handleDeadProcesses(Configs &configs) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        auto [cfg, p] = configs.findByPid(pid);
        if (!p)
            continue;
        cfg->onExit(*p, status);

        int fd = p->waiting_client;
        if (fd != -1) {
            p->waiting_client = -1;
            bool othersPending = false;
            // we only send a reply once all the processes of a config are dead.
            for (auto &q : cfg->programs)
                if (q.waiting_client == fd)
                    othersPending = true;
            if (!othersPending) {
                reply(fd, p->state == State::Starting ? "restarted\n" : "stopped\n");
                if (cfg->isExiled()) {
                    configs.programs.erase(cfg->getName());
                }
            }
        }
    }
}

void readAttachedClients(std::vector<pollfd> &fds, std::vector<pollfd> &pfds,
                         std::vector<AttachedClients> &attachedClients) {
    for (size_t i = 0; i < fds.size(); ++i)
        fds[i].revents = pfds[i].revents;
    for (size_t k = 0; k < attachedClients.size(); ++k)
        if (pfds[fds.size() + k].revents & (POLLIN | POLLHUP | POLLERR)) {
            AttachedClients &w = attachedClients[k];
            if (w.isErr)
                w.cfg->onErrReadable(*w.p);
            else
                w.cfg->onMasterReadable(*w.p);
        }
}

void handleAttachedInput(program &p, const char *buf, ssize_t n) {
    const char *key = static_cast<const char *>(memchr(buf, DETACH_KEY, n));
    ssize_t len = key ? key - buf : n;
    if (len > 0)
        write(p.master_fd, buf, len);
    if (key) {
        reply(p.attached_client, std::string(1, DETACH_KEY));
        p.attached_client = -1;
    }
}

int readClientInputs(std::vector<pollfd> &fds, Configs &configs) {
    for (size_t i = fds.size(); i-- > 1;) {
        if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
            continue;

        int client_fd = fds[i].fd;
        char buf[256] = {0};
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);

        if (n <= 0) {
            std::cout << "Client disconnected (fd=" << client_fd << ")\n";
            configs.forgetClient(client_fd);
            close(client_fd);
            fds.erase(fds.begin() + i);
            continue;
        }

        // input is attached program
        if (program *ap = configs.findAttached(client_fd)) {
            handleAttachedInput(*ap, buf, n);
            continue; // never parsed as a command
        }

        std::string cmd(buf);
        int err = handleCommands(client_fd, cmd, configs);
        if (err == SHUTDOWN) {
            cleanup(fds, configs);
            return 0;
        } else if (err == CLIENT_DISCONNECT) {
            std::cout << "Client disconnected (fd=" << client_fd << ")\n";
            configs.forgetClient(client_fd);
            close(client_fd);
            fds.erase(fds.begin() + i);
        }
    }
    return 1;
}