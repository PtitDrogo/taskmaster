#include "signal.hpp"

volatile sig_atomic_t child_exited = 0;
volatile sig_atomic_t stop_requested = 0;
volatile sig_atomic_t reload_requested = 0;

void sigchld_handler(int) { child_exited = 1; }
void stop_handler(int) { stop_requested = 1; }
void reload_handler(int) { reload_requested = 1; }

void setup_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, nullptr);

    struct sigaction st{};
    st.sa_handler = stop_handler;
    sigemptyset(&st.sa_mask);
    sigaction(SIGINT, &st, nullptr);
    sigaction(SIGTERM, &st, nullptr);

    struct sigaction sh{};
    sh.sa_handler = reload_handler;
    sigemptyset(&sh.sa_mask);
    sigaction(SIGHUP, &sh, nullptr);

    signal(SIGPIPE, SIG_IGN);
}