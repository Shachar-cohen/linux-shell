#ifndef SIGNAL_UTILS_H
#define SIGNAL_UTILS_H

void handler(int sig_num);
void sigxcp_handler(int signum);
void sigsegv_handler(int signum);
void sigxfsz_handler(int signum);
void signofile_handler(int signum);

#endif