// receiver.c - RSSI 포함 8바이트 수신 버전
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define PORT "/dev/ttyAMA0"
#define RX_FRAME_SIZE 29// 수신할 프레임 크기: 마커(1) + 시퀀스(1) + 페이로드(5) + RSSI(1)
#define MARKER_BYTE 0x7E

int main() {
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

    // Raw 모드 (즉각적인 바이트 수신) 설정
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    // read()가 1바이트 수신 시 즉시 리턴하도록 설정
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIFLUSH); // 수신 버퍼 비우기

    unsigned char byte;
    unsigned char frame_buffer[RX_FRAME_SIZE];
    int pos = 0;
    int in_sync = 0;

    //PDR variables
    unsigned long long recieved_count =0; // 수신한 총 패킷 수
    unsigned char max_sequence = 0; //수신한 최고 시퀀스 번호

    printf("🔍 Listening on %s (expecting %d-byte frames with RSSI)...\n", PORT, RX_FRAME_SIZE-1);

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
                unsigned char rssi = frame_buffer[RX_FRAME_SIZE-1];
                // PDR calc logic
                recieved_count++;
                if(seq > max_sequence){
                    max_sequence = seq; // 최고 시퀀스 번호 업데이트
                }
                double pdr = 0.0;
                if(max_sequence > 0 || recieved_count > 0){
                    pdr = ((double)recieved_count  / (max_sequence + 1)) * 100.0;
                }

                printf(" <- Recv'd Frame: ");
                printf("Marker(0x%02X) Seq(0x%02X) Payload(", frame_buffer[0], seq);
                for (int i = 2; i < RX_FRAME_SIZE - 1; i++) {
                    printf("0x%02X ", frame_buffer[i]);
                }
                //rssi & pdr
                printf(") RSSI(0x%02X -> -%d dBm)\n", rssi, 256 - rssi );
                printf(" | PDR %.2f%%  (%llu/%u \n )", pdr, recieved_count, max_sequence+1);
                in_sync = 0;
                pos = 0;
            }
        }
    }

    close(fd);
    return 0;
}
