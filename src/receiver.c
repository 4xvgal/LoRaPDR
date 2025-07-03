// receiver.c - 16진수 출력 및 PDR 계산 포함
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
#define MAX_SEQ 256
#define FRAME_SIZE 6

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

    unsigned char seen[MAX_SEQ] = {0};
    int total_received = 0;
    int unique_received = 0;

    while (1) {
        unsigned char buffer[FRAME_SIZE];
        int n = read(fd, buffer, FRAME_SIZE);
        if (n == FRAME_SIZE) {
            unsigned char seq = buffer[0];

            if (!seen[seq]) {
                seen[seq] = 1;
                unique_received++;
            }
            total_received++;

            // 가장 높은 seq 기준 PDR 계산
            int max_seq = 0;
            for (int i = 0; i < MAX_SEQ; ++i)
                if (seen[i]) max_seq = i;

            float pdr = (float)unique_received / (max_seq + 1) * 100;

            printf("Received frame_seq: 0x%02X | Payload: ", seq);
            for (int i = 1; i < FRAME_SIZE; i++) {
                printf("0x%02X ", buffer[i]);
            }
            printf("| Unique: %d | Total: %d | PDR: %.2f%%\n",
                   unique_received, total_received, pdr);
        }
    }

    close(fd);
    return 0;
}