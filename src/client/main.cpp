#include "client.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/supervisor.sock"

static int handler(void *user, const char *section, const char *name, const char *value) {
    auto *cfg = static_cast<ClientConfig *>(user);
    std::string sect(section), setting(name), val(value);
    int err = 1;
    if (sect == "supervisorctl") {
        err = cfg->parseSetting(setting, val);
    }
    return err;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Error: argument expected" << std::endl;
    }

    ClientConfig config;

    int result = ini_parse(argv[1], handler, &config);
    if (result < 0) {
        std::cerr << "Could not open config file\n";
        return 1;
    } else if (result > 0) {
        std::cerr << "Parse error on line " << result << "\n";
        return 1;
    }

    config.printSettings();

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    char *line;
    while ((line = readline("supervisor> ")) != nullptr) {
        std::string input(line);
        if (!input.empty()) {
            add_history(line);
        }
        free(line);

        if (input == "help") {
            std::cout << "HELP - SHUTDOWN - OTHER STUFF" << std::endl;
        } else if (input.empty()) {
            continue;
        } else {
            write(fd, input.c_str(), input.size());
            std::cout << "Sending" << input.c_str() << std::endl;

            char buf[256];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                std::cout << buf;
            }

        }
    }

    clear_history(); // This is the clear history for the shell.
    close(fd);
    return 0;
}