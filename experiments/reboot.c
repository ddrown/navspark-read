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
#include <string.h>

#define BINARY_MSG_START1 0xA0
#define BINARY_MSG_START2 0xA1
#define BINARY_MSG_END1 0x0D
#define BINARY_MSG_END2 0x0A

#define REBOOT_MSG_LEN 15
#define ACK_MSG_LEN 2

#define MSG_ID_REBOOT 0x01
#define MSG_ID_ACK 0x83
#define MSG_ID_NACK 0x84

#define REBOOT_HOT  0x01
#define REBOOT_WARM 0x02
#define REBOOT_COLD 0x03

#define DEFAULT_CHECKSUM 0x00

#define ID_POS 4
#define MSG_POS(x) (ID_POS + x - 1)

void add_checksum(char *msg, int len) {
  int i;
  char checksum = 0;

  for(i = ID_POS; i <= len+ID_POS; i++) {
    checksum ^= msg[i];
  }
  msg[MSG_POS(len+1)] = checksum;
}

void reboot(int fd) {
  uint8_t buf[32];
  char reboot_msg[] = { BINARY_MSG_START1, BINARY_MSG_START2, (REBOOT_MSG_LEN >> 8), (REBOOT_MSG_LEN & 0xFF), MSG_ID_REBOOT, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    DEFAULT_CHECKSUM, BINARY_MSG_END1, BINARY_MSG_END2 };
  char ack_msg[9];
  char expected_ack[] = { BINARY_MSG_START1, BINARY_MSG_START2, ACK_MSG_LEN >> 8, ACK_MSG_LEN & 0xFF, MSG_ID_ACK, MSG_ID_REBOOT, DEFAULT_CHECKSUM, BINARY_MSG_END1, BINARY_MSG_END2 };
  time_t now;
  struct tm now_info;
  int i;

  add_checksum(expected_ack, ACK_MSG_LEN);

  now = time(NULL);
  gmtime_r(&now, &now_info);

  reboot_msg[MSG_POS(2)] = REBOOT_COLD;

  now_info.tm_year += 1900;
  reboot_msg[MSG_POS(3)] = now_info.tm_year >> 8;
  reboot_msg[MSG_POS(4)] = now_info.tm_year & 0xFF;
  reboot_msg[MSG_POS(5)] = now_info.tm_mon + 1;
  reboot_msg[MSG_POS(6)] = now_info.tm_mday;
  reboot_msg[MSG_POS(7)] = now_info.tm_hour;
  reboot_msg[MSG_POS(8)] = now_info.tm_min;
  reboot_msg[MSG_POS(9)] = now_info.tm_sec;

  add_checksum(reboot_msg, REBOOT_MSG_LEN);

  for(i = 0; i < sizeof(reboot_msg); i++) {
    printf("%02hhx ",reboot_msg[i]);
  }
  printf("\n");

  // System Restart (ID: 0x01) -- page 11
  write(fd, reboot_msg, sizeof(reboot_msg));

  int pos = 0;
  while(pos < sizeof(ack_msg)) {
    // ack/nack msg (ID: 0x83) -- page 84/95
    ssize_t len = read(fd, ack_msg+pos, sizeof(ack_msg)-pos+1);
    if(len > 0) {
      pos += len;
    }
    if(len <= 0) {
      perror("read");
      exit(1);
    }
  }

  if(memcmp(ack_msg, expected_ack, sizeof(expected_ack)) == 0) {
    printf("ack\n");
    exit(0);
  } else {
    for(i = 0; i < pos; i++) {
      printf("%02hhx ",ack_msg[i]);
    }
    printf("\n");
  }
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
    usleep(550000);
    ssize_t len = read(fd, buf, sizeof(buf)); // 1HZ position update

    reboot(fd);
  }
}
