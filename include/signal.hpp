#ifndef SIGNAL_HPP
# define SIGNAL_HPP

#include <csignal>



extern volatile sig_atomic_t child_exited;
extern volatile sig_atomic_t stop_requested;
extern volatile sig_atomic_t reload_requested;


void sigchld_handler(int);
void stop_handler(int);
void reload_handler(int);

void setup_signal_handlers();



#endif