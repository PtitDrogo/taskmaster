#include "utils.hpp"

void reply(int fd, const std::string &msg) {
    if (fd == -1)
        return;
    write(fd, msg.c_str(), msg.size());
}