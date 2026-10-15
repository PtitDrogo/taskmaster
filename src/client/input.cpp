#include "client.hpp"
#include "utils.hpp"
#include <poll.h>
#include <termios.h>
#include <unistd.h>

void readResponseAndPrint(int fd) {
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << buf;
    }
}

void attachSession(int fd) {
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof buf);
    if (n <= 0)
        return;
    std::string resp(buf, n);
    const std::string head = "ATTACHED\n";
    if (resp.compare(0, head.size(), head) != 0) {
        std::cout << resp;
        return;
    } // an ERROR

    termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    cfmakeraw(&raw);
    raw.c_oflag |= OPOST | ONLCR; //\r\n then becomes \r\r\n
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    write(STDOUT_FILENO, "[attached, Ctrl-] to detach]\r\n", 30);

    bool done = false;
    auto show = [&](const char *p, size_t len) {
        const char *key = static_cast<const char *>(memchr(p, DETACH_KEY, len));
        write(STDOUT_FILENO, p, key ? key - p : len);
        if (key)
            done = true;
    };
    show(resp.data() + head.size(), resp.size() - head.size()); // scrollback

    pollfd pfds[2] = {{STDIN_FILENO, POLLIN, 0}, {fd, POLLIN, 0}};
    while (!done) {
        if (poll(pfds, 2, -1) < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (pfds[0].revents & POLLIN) {
            n = read(STDIN_FILENO, buf, sizeof buf);
            if (n <= 0)
                break;
            write(fd, buf, n); 
        }
        if (pfds[1].revents & (POLLIN | POLLHUP)) {
            n = read(fd, buf, sizeof buf);
            if (n <= 0)
                break;
            show(buf, n);
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &orig); 
    std::cout << "\n[detached]\n";
}