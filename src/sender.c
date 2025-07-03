// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyAMA0"
#define PAYLOAD "DUMMY"
#define DELAY_SEC 10

int main() {
    int fd = open(PORT, O_WRONLY | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tcsetattr(fd, TCSANOW, &tty);

    unsigned char frame_seq = 0;
    while (1) {
        char buffer[6];
        buffer[0] = frame_seq;
        memcpy(&buffer[1], PAYLOAD, 5);
        write(fd, buffer, 6);
        printf("Sent frame_seq: %d\n", frame_seq);
        frame_seq++;
        usleep(DELAY_SEC * 1000000);
    }

    close(fd);
    return 0;
}