#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyAMA0"
#define BUF_SIZE 64      // 한 번에 읽을 최대 바이트 수
#define FRAME_SIZE 7     // frame_seq + 5 payload + RSSI

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

    printf("Listening on %s... Press Ctrl+C to exit.\n", PORT);

    while (1) {
        int n = read(fd, buf, BUF_SIZE);
        if (n > 0) {
            printf("Received %d bytes\n", n);

            // 7바이트씩 프레임 파싱
            for (int i = 0; i + FRAME_SIZE - 1 < n; i += FRAME_SIZE) {
                unsigned char seq = buf[i];
                unsigned char rssi = buf[i + FRAME_SIZE - 1];

                printf("  Frame: seq=0x%02X | payload=", seq);
                for (int j = 1; j < FRAME_SIZE - 1; j++) {
                    printf("0x%02X ", buf[i + j]);
                }
                printf("| RSSI: -%d dBm\n", rssi);
            }

            // 남은 바이트 경고 출력 (패킷 중간 잘림 가능성)
            int leftover = n % FRAME_SIZE;
            if (leftover != 0) {
                printf(" Warning: %d leftover byte(s) ignored: ", leftover);
                for (int i = n - leftover; i < n; i++) {
                    printf("0x%02X ", buf[i]);
                }
                printf("\n");
            }
        }

        usleep(10000); // 10ms 대기
    }

    close(fd);
    return 0;
}