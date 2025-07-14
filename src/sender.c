// sender.c - 가변 페이로드 크기 전송 버전
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
#define DELAY_SEC 5
#define MARKER_BYTE 0x7E
#define MAX_PAYLOAD_SIZE 250 // 버퍼 오버플로우 방지를 위한 최대 페이로드 크기 제한

int main(int argc, char *argv[]) {
    // 1. 명령줄 인수 파싱
    if (argc != 3 || strcmp(argv[1], "-payload") != 0) {
        fprintf(stderr, "Usage: %s -payload <size>\n", argv[0]);
        return 1;
    }

    int payload_size = atoi(argv[2]);
    if (payload_size <= 0 || payload_size > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "Error: Payload size must be between 1 and %d.\n", MAX_PAYLOAD_SIZE);
        return 1;
    }

    const int TX_FRAME_SIZE = payload_size + 2; // 전송할 프레임 크기: 마커(1) + 시퀀스(1) + 페이로드(N)

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
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &tty);

    unsigned char frame_seq = 0;
    printf("🚀 Starting sender on %s (sending %d-byte payload, total frame %d bytes)...\n",
           PORT, payload_size, TX_FRAME_SIZE);

    while (1) {
        unsigned char buffer[TX_FRAME_SIZE]; // VLA(Variable-Length Array) 사용
        buffer[0] = MARKER_BYTE;    // 마커
        buffer[1] = frame_seq;      // 시퀀스 번호

        // 페이로드 채우기
        for (int i = 0; i < payload_size; i++) {
            buffer[i + 2] = (unsigned char)i; // 0, 1, 2, ...
        }

        ssize_t bytes_written = write(fd, buffer, TX_FRAME_SIZE);
        if (bytes_written < 0) {
            perror("write failed");
        } else {
            printf(" -> Sent Frame (size: %ld): ", bytes_written);
            printf("Marker(0x%02X) Seq(0x%02X) Payload(%d bytes: ", buffer[0], buffer[1], payload_size);
            for (int i = 2; i < TX_FRAME_SIZE; i++) {
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