#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>

int set_rr(int priority) {
  struct sched_param param;

  param.sched_priority = priority;
  return sched_setscheduler(0, SCHED_RR, &param);
}


int main() {
  char buf[512];
  struct timeval now, last_dollar;
  int found_newline = 1;

  set_rr(32);

  while(1) {
    ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
    gettimeofday(&now, NULL);
    if(len <= 0) {
      exit(0);
    }
    buf[len] = '\0';
    if(found_newline) {
      if(len == 1 && buf[0] == '$' && found_newline == 1) {
        found_newline = 2; // look for a T
        last_dollar.tv_sec = now.tv_sec;
        last_dollar.tv_usec = now.tv_usec;
      } else {
        if(buf[0] == 'T' && found_newline == 2) {
          printf("%0.6f ", (last_dollar.tv_sec + ((double)last_dollar.tv_usec /1000000)));
        }
        found_newline = 0;
      }
    }
    char *newline = strchr(buf, '\n');
    if(newline != NULL && newline == (buf+len-1)) {
      found_newline = 1;
    }
    printf("%s", buf);
  }
}
