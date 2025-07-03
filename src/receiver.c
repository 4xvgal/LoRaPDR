#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyAMA0"
#define FRAME_SIZE 8        // 1 marker + 7 data (seq + 5 payload + rssi)
#define MARKER_BYTE 0x7E    // 프레임 시작 마커

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

    unsigned char byte;
    unsigned char frame[FRAME_SIZE];
    int pos = 0;
    int syncing = 0;

    printf("🔍 Listening on %s (marker: 0x%02X)...\n", PORT, MARKER_BYTE);

    while (1) {
        int n = read(fd, &byte, 1);  // 1바이트씩 읽기
        if (n <= 0) continue;

        if (!syncing) {
            if (byte == MARKER_BYTE) {
                frame[0] = byte;
                pos = 1;
                syncing = 1;
            }
        } else {
            frame[pos++] = byte;
            if (pos == FRAME_SIZE) {
                // 완전한 프레임 수신됨
                unsigned char seq = frame[1];
                unsigned char rssi = frame[7];

                printf("  ✅ Frame: seq=0x%02X | payload=", seq);
                for (int i = 2; i < 7; i++)
                    printf("0x%02X ", frame[i]);
                printf("| RSSI: -%d dBm\n", rssi);

                // 다음 프레임 대기
                syncing = 0;
                pos = 0;
            }
        }
    }

    close(fd);
    return 0;
}