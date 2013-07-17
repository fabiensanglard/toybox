/* timeout.c - Run command line with a timeout
 *
 * Copyright 2013 Rob Landley <rob@landley.net>
 *
 * No standard

USE_TIMEOUT(NEWTOY(timeout, "<2^k:s: ", TOYFLAG_BIN))

config TIMEOUT
  bool "timeout"
  default y
  depends on TOYBOX_FLOAT
  help
    usage: timeout [-k LENGTH] [-s SIGNAL] LENGTH COMMAND...

    Run command line as a child process, sending child a signal if the
    command doesn't exit soon enough.

    Length can be a decimal fraction. An optional suffix can be "m"
    (minutes), "h" (hours), "d" (days), or "s" (seconds, the default).

    -s	Send specified signal (default TERM)
    -k	Send KILL signal if child still running this long after first signal.
*/

#define FOR_timeout
#include "toys.h"

GLOBALS(
  char *s_signal;
  char *k_timeout;

  int nextsig;
  pid_t pid;
  struct timeval ktv;
  struct itimerval itv;
)

static void handler(int i)
{
  kill(TT.pid, TT.nextsig);
  
  if (TT.k_timeout) {
    TT.k_timeout = 0;
    TT.nextsig = SIGKILL;
    signal(SIGALRM, handler);
    TT.itv.it_value = TT.ktv;
    setitimer(ITIMER_REAL, &TT.itv, (void *)&toybuf);
  }
}

void timeout_main(void)
{
  // Parse early to get any errors out of the way.
  TT.itv.it_value.tv_sec = xparsetime(*toys.optargs, 1000000, &TT.itv.it_value.tv_usec);

  if (TT.k_timeout)
    TT.ktv.tv_sec = xparsetime(TT.k_timeout, 1000000, &TT.ktv.tv_usec);
  TT.nextsig = SIGTERM;
  if (TT.s_signal && -1 == (TT.nextsig = sig_to_num(TT.s_signal)))
    error_exit("bad -s: '%s'", TT.s_signal);

  if (!(TT.pid = fork())) xexec_optargs(1);
  else {
    int status;

    signal(SIGALRM, handler);
    setitimer(ITIMER_REAL, &TT.itv, (void *)&toybuf);
    while (-1 == waitpid(TT.pid, &status, 0) && errno == EINTR);
    if (WIFEXITED(status)) toys.exitval = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) toys.exitval = WTERMSIG(status);
    else toys.exitval = 1;
  }
}
