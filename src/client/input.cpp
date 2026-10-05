#include "client.hpp"
#include <unistd.h>

void readResponse(int fd) {
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::cout << buf;
    }
}