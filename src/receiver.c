// receiver.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
#define MAX_SEQ 256

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
        unsigned char buffer[6];
        int n = read(fd, buffer, 6);
        if (n == 6) {
            unsigned char seq = buffer[0];
            if (!seen[seq]) {
                seen[seq] = 1;
                unique_received++;
            }
            total_received++;

            // 가장 높은 frame_seq 기준으로 PDR 계산
            int max_seq = 0;
            for (int i = 0; i < MAX_SEQ; ++i)
                if (seen[i]) max_seq = i;

            float pdr = (float)unique_received / (max_seq + 1) * 100;
            printf("Received seq: %d | Unique: %d | Total: %d | PDR: %.2f%%\n",
                   seq, unique_received, total_received, pdr);
        }
    }

    close(fd);
    return 0;
}