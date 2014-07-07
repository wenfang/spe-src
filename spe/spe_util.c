#include "spe_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/*
===================================================================================================
spe_daemon
===================================================================================================
*/
int 
spe_daemon() {
  switch (fork()) {
  case -1:
    return -1;
  case 0:
    break;
  default:
    exit(EXIT_SUCCESS);
  }

  if (setsid() == -1) return -1;
  if (chdir("/") != 0) return -1;

  int fd;
  if ((fd = open("/dev/null", O_RDWR, 0)) == -1) return -1;
  if (dup2(fd, STDIN_FILENO) < 0) return -1;
  if (dup2(fd, STDOUT_FILENO) < 0) return -1;
  if (dup2(fd, STDERR_FILENO) < 0) return -1;
  if (fd > STDERR_FILENO) close(fd);
  return 0;
}

/*
===================================================================================================
spe_spawn
===================================================================================================
*/
int
spe_spawn(unsigned procs) {
  for (int i=1; i<procs; i++) {
    pid_t pid = fork();
    if (!pid) return 0; // child return here
    if (pid < 0) return -1;
  }
  return 1;
}

/*
===================================================================================================
spe_save_pid
===================================================================================================
*/
bool
spe_save_pid(const char* pid_file) {
  ASSERT(pid_file);
  FILE *fp;
  if (!(fp = fopen(pid_file, "w"))) return false;
  fprintf(fp, "%ld\n", (long)getpid());
  if (fclose(fp) == -1) return false;
  return true;
}

/*
===================================================================================================
spe_remove_pid
===================================================================================================
*/
bool
spe_remove_pid(const char* pid_file) {
  ASSERT(pid_file);
  if (unlink(pid_file)) return false;
  return true;
}
