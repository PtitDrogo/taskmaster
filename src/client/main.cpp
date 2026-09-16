#include "ini.h"
#include "taskmaster.hpp"

int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     std::cerr << ""
    // }
    (void)argc;
    (void)argv;
    // I must read the config file first, then the server will start the processes and keep track of them.
    // I should only need like a PID and a socket number i guess ?
    //  Its a process, I gotta remember how to talk to one, theres a pipe right ?

    // int result = ini_parse("supervisor.conf", handler, &configs);
    // if (result < 0) {
    //     std::cerr << "Could not open config file\n";
    //     return 1;
    // } else if (result > 0) {
    //     std::cerr << "Parse error on line " << result << "\n";
    //     return 1;
    // }

    std::cout << "I am the client !" << std::endl;
    return 0;
}