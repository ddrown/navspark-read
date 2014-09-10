#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <stdint.h>
#include <time.h>

struct gps_time {
  uint32_t tow;
  uint32_t stow;
  uint16_t wn;
  int8_t defleap;
  int8_t curleap;
  uint8_t valid;
};

void querytime(int fd) {
  uint8_t buf[32];
  char query_time[] = { 0xA0, 0xA1, 0x00, 0x02, 0x64, 0x20, 0x44, 0x0D, 0x0A };
  struct timeval start,now;
  struct gps_time official_time;
  uint32_t official_epoch;
  long nsec;

  // QUERY GPS TIME - Query GPS time of GNSS receiver (ID: 0x64, SID: 0x20) -- page 59
  write(fd, query_time, sizeof(query_time));
  gettimeofday(&start, NULL);

  int pos = 0;
  while(pos < sizeof(buf)) {
    // GPS TIME – GPS time of GNSS receiver (ID: 0x64, SID: 0x8E)  -- page 65
    ssize_t len = read(fd, buf+pos, sizeof(buf)-pos+1);
    gettimeofday(&now, NULL);
    if(len > 0) {
      pos += len;
    }
  }

  official_time.tow = ((uint32_t)buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + buf[19];
  official_time.stow = ((uint32_t)buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + buf[23];
  official_time.wn = ((uint16_t)buf[24] << 8) + buf[25];
  official_time.defleap = (int8_t)buf[26];
  official_time.curleap = (int8_t)buf[27];
  official_time.valid = buf[28];

  official_epoch = (uint32_t)315964800 + ((uint32_t)official_time.wn *7*24*60*60) + (official_time.tow / 1000) - official_time.defleap;
  nsec = (official_time.tow % 1000) * 1000000 + official_time.stow;

  printf("start=%0.6f end=%0.6f gps=%0.9f\n", (start.tv_sec + ((double)start.tv_usec/1000000)), (now.tv_sec + ((double)now.tv_usec/1000000)), (official_epoch + ((double)nsec/1000000000)));
}

int main() {
  char buf[1024];
  int fd;
  fd_set readfds;

  fd = open("/dev/ttyUSB0", O_RDWR);
  if(fd < 0) {
    perror("/dev/ttyUSB0 open");
    exit(1);
  }

  while(1) {
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    select(fd+1, &readfds, NULL, NULL, NULL);
    usleep(250000);
    ssize_t len = read(fd, buf, sizeof(buf)); // 1HZ position update
/*    gettimeofday(&now, NULL);
    if(len > 0) {
      buf[len] = '\0';
      printf("%0.6f [%d] %s\n", (now.tv_sec + ((double)now.tv_usec/1000000)), len, buf);
    } */

    querytime(fd);
  }
}
