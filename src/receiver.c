#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyAMA0"
#define BUF_SIZE 64  // 한 번에 읽을 최대 바이트 수

int main() {
    int fd = open(PORT, O_RDONLY | O_NOCTTY);
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

    unsigned char buf[BUF_SIZE];

    printf("🔍 Listening on %s... Press Ctrl+C to exit.\n", PORT);

    while (1) {
        int n = read(fd, buf, BUF_SIZE);
        if (n > 0) {
            printf("Received %d bytes: ", n);
            for (int i = 0; i < n; ++i) {
                printf("0x%02X ", buf[i]);
            }
            printf("\n");
        }
        usleep(10000); // 10ms 쉬어줌 (optional)
    }

    close(fd);
    return 0;
}