// sender.c - 실제 바이트 (16진수 값) 전송 버전
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyAMA0"
#define DELAY_SEC 10
#define MARKER_BYTE 0x7E

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
        unsigned char buffer[7];
        buffer[0] = 0x7E;  // 마커
        buffer[1] = frame_seq;       // 시퀀스 번호
        // payload: 0xA1, 0xB2, 0xC3, 0xD4, 0xE5 (예시)
        buffer[2] = 0xA1;
        buffer[3] = 0xB2;
        buffer[4] = 0xC3;
        buffer[5] = 0xD4;
        buffer[6] = 0xE5;
        write(fd, buffer, 7);

        printf("Sent frame_seq: 0x%02X | Payload: ", frame_seq);
        for (int i = 1; i < 6; i++) {
            printf("0x%02X ", buffer[i]);
        }
        printf("\n");

        frame_seq++;
        usleep(DELAY_SEC * 1000000);
    }

    close(fd);
    return 0;
}