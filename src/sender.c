// sender.c - RSSI 없이 순수 데이터(7바이트)만 전송하는 버전
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
#define DELAY_SEC 5
#define PAYLOAD_SIZE 5
// #define TX_FRAME_SIZE 42    // 전송할 프레임 크기: 마커(1) + 시퀀스(1) + 페이로드(5)
#define MARKER_BYTE 0x7E


int main() {
    int TX_FRAME_SIZE = PAYLOAD_SIZE + 2;

    int fd = open(PORT, O_WRONLY | O_NOCTTY);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr failed");
        close(fd);
        return 1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS; // 하드웨어 흐름 제어 비활성화
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &tty);

    unsigned char frame_seq = 0;
    printf("🚀 Starting sender on %s (sending %d-byte frames)...\n", PORT, TX_FRAME_SIZE);

    while (1) {
        unsigned char buffer[TX_FRAME_SIZE];
        buffer[0] = MARKER_BYTE;    // 마커
        buffer[1] = frame_seq;      // 시퀀스 번호
        // 페이로드: 0xA1, 0xB2, 0xC3, 0xD4, 0xE5
        // buffer[2] = 0xA1;
        // buffer[3] = 0xB2;
        // buffer[4] = 0xC3;
        // buffer[5] = 0xD4;
        // buffer[6] = 0xE5;
        int j = 0;
        for(int i=2; i< PAYLOAD_SIZE+2;i++){     
            buffer[i] = j++;
            
        }
        ssize_t bytes_written = write(fd, buffer, TX_FRAME_SIZE);
        if (bytes_written < 0) {
            perror("write failed");
        } else {
            printf(" -> Sent Frame (size: %ld): ", bytes_written);
            printf("Marker(0x%02X) Seq(0x%02X) Payload(", buffer[0], buffer[1]);
            for (int i = 2; i <TX_FRAME_SIZE; i++) {
                printf("0x%02X ", buffer[i]);
            }
            printf(")\n");
        }

        frame_seq++;
        sleep(DELAY_SEC);
    }

    close(fd);
    return 0;
}