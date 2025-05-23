#include <wiringPi.h>
#include <softTone.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define SPKR 5
#define TOTAL_NOTE 32

int notes[] = {
    391, 391, 440, 440, 391, 391, 330, 330,
    391, 391, 330, 330, 294, 294, 294, 0,
    391, 391, 440, 440, 391, 391, 330, 330,
    391, 330, 294, 330, 262, 262, 262, 0
};

int runCommandWithSocket(const char *cmd, int client_fd) {
    if (strcmp(cmd, "BUZZER PLAY") != 0) return -1;

    softToneCreate(SPKR);

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    int state = 1; // 1: playing, 0: paused
    int current = 0;
    char buf[2] = {0};
    char menu[128];

    snprintf(menu, sizeof(menu),
        "===재생중===\n1. buzzer OFF\n2. 메뉴 화면으로\n");
    write(client_fd, menu, strlen(menu));

    while (current < TOTAL_NOTE) {
        ssize_t len = read(client_fd, buf, 1);
        if (len == 0) {
            // 클라이언트가 연결을 끊음
            break;
        } else if (len > 0) {
            if (buf[0] == '1') {
                state = !state;
                const char *resumed =
                    state ? "===재생중===\n1. buzzer OFF\n2. 메뉴 화면으로\n"
                          : "===일시정지됨===\n1. buzzer ON\n2. 메뉴 화면으로\n";
                write(client_fd, resumed, strlen(resumed));
            } else if (buf[0] == '2') {
                softToneWrite(SPKR, 0);
                write(client_fd, "BUZZER 종료\n", strlen("BUZZER 종료\n"));
                return 0;
            }
        }

        if (state == 1) {
            softToneWrite(SPKR, notes[current]);
            delay(280);
            current++;
        } else {
            softToneWrite(SPKR, 0); // 일시정지 중에는 소리를 멈춘다
            delay(100);
        }
    }

    softToneWrite(SPKR, 0);
    write(client_fd, "BUZZER 재생 완료\n", strlen("BUZZER 재생 완료\n"));
    return 0;
}
