#include "utils.hpp"

void reply(int fd, const std::string &msg) { write(fd, msg.c_str(), msg.size()); }