// receiver.c - RSSI 포함, 가변 페이로드 크기 수신 버전
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
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

    const int RX_FRAME_SIZE = payload_size + 3; // 수신할 프레임 크기: 마커(1) + 시퀀스(1) + 페이로드(N) + RSSI(1)

    int fd = open(PORT, O_RDONLY | O_NOCTTY);
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

    // Raw 모드 설정
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIFLUSH);

    unsigned char frame_buffer[RX_FRAME_SIZE]; // VLA(Variable-Length Array) 사용
    unsigned char byte;
    int pos = 0;
    int in_sync = 0;

    // PDR 변수
    unsigned long long received_count = 0;
    unsigned char max_sequence = 0;

    printf("🔍 Listening on %s (expecting %d-byte payload, total frame %d bytes with RSSI)...\n",
           PORT, payload_size, RX_FRAME_SIZE);

    while (1) {
        int n = read(fd, &byte, 1);
        if (n < 0) {
            perror("read failed");
            break;
        }
        if (n == 0) continue;

        if (!in_sync) {
            if (byte == MARKER_BYTE) {
                frame_buffer[0] = byte;
                pos = 1;
                in_sync = 1;
            }
        } else {
            frame_buffer[pos++] = byte;
            if (pos >= RX_FRAME_SIZE) {
                unsigned char seq = frame_buffer[1];
                unsigned char rssi = frame_buffer[RX_FRAME_SIZE - 1];
                
                // PDR 계산 로직
                received_count++;
                if (seq > max_sequence) {
                    max_sequence = seq;
                }
                double pdr = 0.0;
                if (max_sequence > 0 || received_count > 0) {
                    pdr = ((double)received_count / (max_sequence + 1)) * 100.0;
                }

                printf(" <- Recv'd Frame: ");
                printf("Marker(0x%02X) Seq(0x%02X) Payload(%d bytes: ", frame_buffer[0], seq, payload_size);
                for (int i = 2; i < RX_FRAME_SIZE - 1; i++) {
                    printf("0x%02X ", frame_buffer[i]);
                }
                printf(") RSSI(0x%02X -> -%d dBm)", rssi, 256 - rssi);
                printf(" | PDR: %.2f%% (%llu/%u)\n", pdr, received_count, max_sequence + 1);
                
                in_sync = 0;
                pos = 0;
            }
        }
    }

    close(fd);
    return 0;
}